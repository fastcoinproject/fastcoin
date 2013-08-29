// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "protocol.h"
#include "udp.h"
#include "net.h"
#include "main.h"
#include "serialize.h"

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

using boost::asio::ip::udp;

// runs inside cs_main
static bool ProcessUDPMessage(CNode *pfrom, string strCommand, CDataStream& vRecv)
{
    // process some commands as if received via TCP
    if (strCommand == "addr" ||
        strCommand == "inv" ||
        strCommand == "tx")
        return ProcessMessage(pfrom, strCommand, vRecv);

    // UDP-specific processing
    if (strCommand == "udpsub")
    {
        uint64 mask, cookie;
        vRecv >> mask;
        vRecv >> cookie;

        // validate cookie from "udpcook" message
        if (!cookie || cookie != pfrom->nUDPCookie)
            return true;

        // set subscription mask
        pfrom->nUDPSubMask = mask;
    }

    // ignore unknown commands, for extensibility
    else {
        // do nothing
    }

    return true;
}

static bool ProcessUDPMessages(CNode *pfrom, const char *data, size_t data_len,
                               CDataStream &vSend)
{
    CDataStream vRecv(data, data + data_len, SER_NETWORK, PROTOCOL_VERSION);
    if (vRecv.empty())
        return true;
    if (fDebug)
        printf("ProcessUDPMessages(%u bytes)\n", vRecv.size());

    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //

    loop
    {
        // Don't bother if send buffer is too full to respond anyway
        if (vSend.size() >= SendBufferSize())
            break;

        // Scan for message start
        CDataStream::iterator pstart = search(vRecv.begin(), vRecv.end(), BEGIN(pchMessageStart), END(pchMessageStart));
        int nHeaderSize = vRecv.GetSerializeSize(CMessageHeader());
        if (vRecv.end() - pstart < nHeaderSize)
        {
            if ((int)vRecv.size() > nHeaderSize)
            {
                printf("\n\nPROCESSUDPMESSAGE MESSAGESTART NOT FOUND\n\n");
                vRecv.erase(vRecv.begin(), vRecv.end() - nHeaderSize);
            }
            break;
        }
        if (pstart - vRecv.begin() > 0)
            printf("\n\nPROCESSUDPMESSAGE SKIPPED %PRIpdd BYTES\n\n", pstart - vRecv.begin());
        vRecv.erase(vRecv.begin(), pstart);

        // Read header
        vector<char> vHeaderSave(vRecv.begin(), vRecv.begin() + nHeaderSize);
        CMessageHeader hdr;
        vRecv >> hdr;
        if (!hdr.IsValid())
        {
            printf("\n\nPROCESSUDPMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand().c_str());
            continue;
        }
        string strCommand = hdr.GetCommand();

        // Message size
        unsigned int nMessageSize = hdr.nMessageSize;
        if (nMessageSize > MAX_SIZE)
        {
            printf("ProcessMessages(%s, %u bytes) : nMessageSize > MAX_SIZE\n", strCommand.c_str(), nMessageSize);
            continue;
        }
        if (nMessageSize > vRecv.size())
        {
            // Message size larger than packet size
            printf("ProcessMessages(%s, %u bytes) : nMessageSize > pkt size\n", strCommand.c_str(), nMessageSize);
            break;
        }

        // Checksum
        uint256 hash = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
        unsigned int nChecksum = 0;
        memcpy(&nChecksum, &hash, sizeof(nChecksum));
        if (nChecksum != hdr.nChecksum)
        {
            printf("ProcessUDPMessages(%s, %u bytes) : CHECKSUM ERROR nChecksum=%08x hdr.nChecksum=%08x\n",
               strCommand.c_str(), nMessageSize, nChecksum, hdr.nChecksum);
            continue;
        }

        // Copy message to its own buffer
        CDataStream vMsg(vRecv.begin(), vRecv.begin() + nMessageSize, vRecv.nType, vRecv.nVersion);
        vRecv.ignore(nMessageSize);

        // Process message
        bool fRet = false;
        try
        {
            {
                LOCK(cs_main);
                fRet = ProcessUDPMessage(pfrom, strCommand, vMsg);
            }
            if (fShutdown)
                return true;
        }
        catch (std::ios_base::failure& e)
        {
            if (strstr(e.what(), "end of data"))
            {
                // Allow exceptions from under-length message on vRecv
                printf("ProcessUDPMessages(%s, %u bytes) : Exception '%s' caught, normally caused by a message being shorter than its stated length\n", strCommand.c_str(), nMessageSize, e.what());
            }
            else if (strstr(e.what(), "size too large"))
            {
                // Allow exceptions from over-long size
                printf("ProcessUDPMessages(%s, %u bytes) : Exception '%s' caught\n", strCommand.c_str(), nMessageSize, e.what());
            }
            else
            {
                PrintExceptionContinue(&e, "ProcessUDPMessages()");
            }
        }
        catch (std::exception& e) {
            PrintExceptionContinue(&e, "ProcessUDPMessages()");
        } catch (...) {
            PrintExceptionContinue(NULL, "ProcessUDPMessages()");
        }

        if (!fRet)
            printf("ProcessMessage(%s, %u bytes) FAILED\n", strCommand.c_str(), nMessageSize);
    }

    return true;
}


class server
{
public:
  server(boost::asio::io_service& io_service, short port)
    : io_service_(io_service),
      socket_(io_service, udp::endpoint(udp::v4(), port))
  {
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&server::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_receive_from(const boost::system::error_code& error,
      size_t bytes_recvd)
  {
    // if we don't know the node via TCP, ignore message
    CNetAddr remote_addr(sender_endpoint_.address().to_string());

   // printf("INBOUND UDP\n");
    printf("INBOUND UDP -> %s",sender_endpoint_.address().to_string().c_str() );

    CNode *pfrom = FindNode(remote_addr);   // FIXME need ref?

    if (pfrom && !error && bytes_recvd > 0)
    {
      CDataStream vSend(SER_NETWORK, PROTOCOL_VERSION);
      ProcessUDPMessages(pfrom, data_, bytes_recvd, vSend);
      if (vSend.size() > 0)
        socket_.async_send_to(
            boost::asio::buffer(&vSend[0], vSend.size()), sender_endpoint_,
            boost::bind(&server::handle_send_to, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
    }

    // fall through:
    // wait for next UDP message
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&server::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_send_to(const boost::system::error_code& /*error*/,
      size_t /*bytes_sent*/)
  {
    // wait for next UDP message
      printf("UDP RECEIVE  ");
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&server::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void sendmsg(udp::endpoint &remote_endpoint,
               const char *data, unsigned int data_len)
  {
    try {
        socket_.async_send_to(
            boost::asio::buffer(data, data_len), remote_endpoint,
            boost::bind(&server::handle_send_to, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
        printf("UDP SEND COMPLETE ");
      }
        catch (std::exception& e)
          {
            std::cerr << e.what() << std::endl;
          }
  }

  void sendmsg(std::string ipAddr, std::string strPort,
               char *data, unsigned int data_len)
  {
      //udp::resolver resolver(io_service_);
      //udp::resolver::query query(udp::v4(), ipAddr, strPort);
      //udp::endpoint receiver_endpoint = *resolver.resolve(query);


//      udp::endpoint receiver_endpoint(
  //                  boost::asio::ip::address::from_string(ipAddr),
    //                boost::lexical_cast<int>(strPort)
      //            );


//          udp::endpoint receiver_endpoint(
  //          boost::asio::ip::address::from_string(ipAddr), boost::lexical_cast<int>(strPort));
   //    socket_.open(boost::asio::ip::udp::v4());
     //  boost::asio::socket_base::broadcast option(true);
   //   std::cout << "ip: " << boost::asio::ip::address::from_string(ipAddr) << " port: " << strPort << std::endl;
       udp::endpoint receiver_endpoint(
                   boost::asio::ip::address::from_string(ipAddr), boost::lexical_cast<int>(strPort));
       //socket.set_option(option);
       //endpoint = boost::asio::ip::udp::endpoint(
         //  boost::asio::ip::address::from_string("192.168.1.255"),
           //port);



       std::cout << "UDP ENDPOINT CREATED -> " << receiver_endpoint << " ";


             // ip::address::from_string(ipAddr)



      sendmsg(receiver_endpoint, data, data_len);
  }

private:
  boost::asio::io_service& io_service_;
  udp::socket socket_;
  udp::endpoint sender_endpoint_;
  enum { max_length = 100 * 1024 };
  char data_[max_length];
};

static class server *cur_server = NULL;

bool SendUDPMessage(CNode *pfrom, string strCommand, CDataStream &vData)
{
    CDataStream vSend(SER_NETWORK, PROTOCOL_VERSION);
    unsigned int nHeaderStart = vSend.size();
    vSend << CMessageHeader(strCommand.c_str(), 0);
    unsigned int nMessageStart = vSend.size();

    vSend << vData;

    // Set the size
    unsigned int nSize = vSend.size() - nMessageStart;
    memcpy((char*)&vSend[nHeaderStart] + CMessageHeader::MESSAGE_SIZE_OFFSET, &nSize, sizeof(nSize));

    // Set the checksum
    uint256 hash = Hash(vSend.begin() + nMessageStart, vSend.end());
    unsigned int nChecksum = 0;
    memcpy(&nChecksum, &hash, sizeof(nChecksum));
    assert(nMessageStart - nHeaderStart >= CMessageHeader::CHECKSUM_OFFSET + sizeof(nChecksum));
    memcpy((char*)&vSend[nHeaderStart] + CMessageHeader::CHECKSUM_OFFSET, &nChecksum, sizeof(nChecksum));

    if (cur_server) {
        cur_server->sendmsg(pfrom->addr.ToStringIP(), pfrom->addr.ToStringPort(),
                           &vSend[0], (unsigned int) vSend.size());

        return true;
    }

    return false;
}

void ThreadUDPServer2(void* parg)
{
    printf("ThreadUDPServer started\n");

    vnThreadsRunning[THREAD_UDP]++;

    try
    {
      boost::asio::io_service io_service;

      server s(io_service, GetListenPort());

      cur_server = &s;

      while (!fShutdown)
          io_service.run_one();

      cur_server = NULL;

    }
    catch (std::exception& e)
    {
        PrintException(&e, "ThreadUDPServer2()");
    }

    vnThreadsRunning[THREAD_UDP]--;
}

void ThreadUDPServer(void* parg)
{
    // Make this thread recognisable as the UDP server thread
    RenameThread("fastcoin-udp");

    try
    {
        ThreadUDPServer2(parg);
    }
    catch (std::exception& e) {
        PrintException(&e, "ThreadUDPServer()");
    }
    printf("ThreadUDPServer exited\n");
}
