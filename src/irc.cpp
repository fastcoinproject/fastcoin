// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2012 Fastcoin Developers
// Copyright (c) 2013 Fastcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// CORE Update

#include "irc.h"
#include "util.h"
#include "timedata.h"

#include "net.h"
#include <string>
#include <stdarg.h>

#include <boost/thread.hpp>

using namespace std;
using namespace boost;

int nGotIRCAddresses = 0;
bool fShutdown = false;

void ThreadIRCSeed2(void* parg);


std::string real_strprintf(const std::string &format, int dummy, ...);
#define strprintf2(format, ...) real_strprintf(format, 0, __VA_ARGS__)

#define loop                for (;;)

#ifndef HAVE_STRLCAT
/*
 * '_cups_strlcat()' - Safely concatenate two strings.
 */

size_t                  /* O - Length of string */
strlcat(char       *dst,        /* O - Destination string */
              const char *src,      /* I - Source string */
          size_t     size)      /* I - Size of destination string buffer */
{
  size_t    srclen;         /* Length of source string */
  size_t    dstlen;         /* Length of destination string */


 /*
  * Figure out how much room is left...
  */

  dstlen = strlen(dst);
  size   -= dstlen + 1;

  if (!size)
    return (dstlen);        /* No room, return immediately... */

 /*
  * Figure out how much room is needed...
  */

  srclen = strlen(src);

 /*
  * Copy the appropriate amount...
  */

  if (srclen > size)
    srclen = size;

  memcpy(dst + dstlen, src, srclen);
  dst[dstlen + srclen] = '\0';

  return (dstlen + srclen);
}
#endif /* !HAVE_STRLCAT */

#ifndef HAVE_STRLCPY
/*
 * '_cups_strlcpy()' - Safely copy two strings.
 */

size_t                  /* O - Length of string */
strlcpy(char       *dst,        /* O - Destination string */
              const char *src,      /* I - Source string */
          size_t      size)     /* I - Size of destination string buffer */
{
  size_t    srclen;         /* Length of source string */


 /*
  * Figure out how much room is needed...
  */

  size --;

  srclen = strlen(src);

 /*
  * Copy the appropriate amount...
  */

  if (srclen > size)
    srclen = size;

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';

  return (srclen);
}
#endif /* !HAVE_STRLCPY */

#pragma pack(push, 1)
struct ircaddr
{
    struct in_addr ip;
    short port;
};
#pragma pack(pop)

void ParseString(const string& str, char c, vector<string>& v)
{
    if (str.empty())
        return;
    string::size_type i1 = 0;
    string::size_type i2;
    loop
    {
        i2 = str.find(c, i1);
        if (i2 == str.npos)
        {
            v.push_back(str.substr(i1));
            return;
        }
        v.push_back(str.substr(i1, i2-i1));
        i1 = i2+1;
    }
}

inline void Sleep2(int64_t n)
{
    /*Boost has a year 2038 problem— if the request sleep time is past epoch+2^31 seconds the sleep returns instantly.
	 So we clamp our sleeps here to 10 years and hope that boost is fixed by 2028.*/
    boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(n>315576000000LL?315576000000LL:n));
}

/*
void RenameThread(const char* name)
{
#if defined(PR_SET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif 0 && (defined(__FreeBSD__) || defined(__OpenBSD__))
    // TODO: This is currently disabled because it needs to be verified to work
    //       on FreeBSD or OpenBSD first. When verified the '0 &&' part can be
    //       removed.
    pthread_set_name_np(pthread_self(), name);
#elif defined(MAC_OSX)
    pthread_setname_np(name);
#else
    // Prevent warnings for unused parameters...
    (void)name;
#endif
}
*/

/*
static std::string FormatException(std::exception* pex, const char* pszThread)
{
#ifdef WIN32
    char pszModule[MAX_PATH] = "";
    GetModuleFileNameA(NULL, pszModule, sizeof(pszModule));
#else
    const char* pszModule = "fastcoin";
#endif
    if (pex)
        return strprintf2(
						 "EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, pszThread);
    else
        return strprintf2(
						 "UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
}


 void PrintExceptionContinue(std::exception* pex, const char* pszThread)
{
    std::string message = FormatException(pex, pszThread);
    printf("\n\n************************\n%s\n", message.c_str());
    fprintf(stderr, "\n\n************************\n%s\n", message.c_str());
    //strMiscWarning = message;
}

static int64_t nMockTime = 0;  // For unit testing

int64_t GetTime()
{
    if (nMockTime) return nMockTime;
	
    return time(NULL);
}

void SetMockTime(int64_t nMockTimeIn)
{
    nMockTime = nMockTimeIn;
}

static int64_t nTimeOffset = 0;

int64_t GetAdjustedTime()
{
    return GetTime() + nTimeOffset;
}
*/

string vstrprintf(const char *format, va_list ap)
{
    char buffer[50000];
    char* p = buffer;
    int limit = sizeof(buffer);
    int ret;
    loop
    {
        va_list arg_ptr;
        va_copy(arg_ptr, ap);
#ifdef WIN32
        ret = _vsnprintf(p, limit, format, arg_ptr);
#else
        ret = vsnprintf(p, limit, format, arg_ptr);
#endif
        va_end(arg_ptr);
        if (ret >= 0 && ret < limit)
            break;
        if (p != buffer)
            delete[] p;
        limit *= 2;
        p = new char[limit];
        if (p == NULL)
            throw std::bad_alloc();
    }
    string str(p, p+ret);
    if (p != buffer)
        delete[] p;
    return str;
}

string real_strprintf(const char *format, int dummy, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, dummy);
    string str = vstrprintf(format, arg_ptr);
    va_end(arg_ptr);
    return str;
}

string real_strprintf(const std::string &format, int dummy, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, dummy);
    string str = vstrprintf(format.c_str(), arg_ptr);
    va_end(arg_ptr);
    return str;
}

///

string EncodeAddress(const CService& addr)
{
    struct ircaddr tmp;
    if (addr.GetInAddr(&tmp.ip))
    {
        tmp.port = htons(addr.GetPort());

        vector<unsigned char> vch(UBEGIN(tmp), UEND(tmp));
        return string("u") + EncodeBase58Check(vch);
    }
    return "";
}

bool DecodeAddress(string str, CService& addr)
{
    vector<unsigned char> vch;
    if (!DecodeBase58Check(str.substr(1), vch))
        return false;

    struct ircaddr tmp;
    if (vch.size() != sizeof(tmp))
        return false;
    memcpy(&tmp, &vch[0], sizeof(tmp));

    addr = CService(tmp.ip, ntohs(tmp.port));
    return true;
}






static bool Send(SOCKET hSocket, const char* pszSend)
{
    if (strstr(pszSend, "PONG") != pszSend)
        printf("IRC SENDING: %s\n", pszSend);
    const char* psz = pszSend;
    const char* pszEnd = psz + strlen(psz);
    while (psz < pszEnd)
    {
        int ret = send(hSocket, psz, pszEnd - psz, MSG_NOSIGNAL);
        if (ret < 0)
            return false;
        psz += ret;
    }
    return true;
}

bool RecvLineIRC(SOCKET hSocket, string& strLine)
{
    loop
    {
        bool fRet = RecvLine(hSocket, strLine);
        if (fRet)
        {
            if (fShutdown)
                return false;
            boost::this_thread::interruption_point();

            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords.size() >= 1 && vWords[0] == "PING")
            {
                strLine[1] = 'O';
                strLine += '\r';
                Send(hSocket, strLine.c_str());
                continue;
            }
        }
        return fRet;
    }
}

int RecvUntil(SOCKET hSocket, const char* psz1, const char* psz2=NULL, const char* psz3=NULL, const char* psz4=NULL)
{
    loop
    {
        string strLine;
        strLine.reserve(10000);
        if (!RecvLineIRC(hSocket, strLine))
            return 0;
        printf("IRC %s\n", strLine.c_str());
        if (psz1 && strLine.find(psz1) != string::npos)
            return 1;
        if (psz2 && strLine.find(psz2) != string::npos)
            return 2;
        if (psz3 && strLine.find(psz3) != string::npos)
            return 3;
        if (psz4 && strLine.find(psz4) != string::npos)
            return 4;
    }
}

bool Wait(int nSeconds)
{
    if (fShutdown)
        return false;
    boost::this_thread::interruption_point();
    printf("IRC waiting %d seconds to reconnect\n", nSeconds);
    for (int i = 0; i < nSeconds; i++)
    {
       if (fShutdown)
            return false;
        boost::this_thread::interruption_point();
        Sleep2(1000);
    }
    return true;
}

bool RecvCodeLine(SOCKET hSocket, const char* psz1, string& strRet)
{
    strRet.clear();
    loop
    {
        string strLine;
        if (!RecvLineIRC(hSocket, strLine))
            return false;

        vector<string> vWords;
        ParseString(strLine, ' ', vWords);
        if (vWords.size() < 2)
            continue;

        if (vWords[1] == psz1)
        {
            printf("IRC %s\n", strLine.c_str());
            strRet = strLine;
            return true;
        }
    }
}

bool GetIPFromIRC(SOCKET hSocket, string strMyName, CNetAddr& ipRet)
{
    Send(hSocket, strprintf2("USERHOST %s\r", strMyName.c_str()).c_str());

    string strLine;
    if (!RecvCodeLine(hSocket, "302", strLine))
        return false;

    vector<string> vWords;
    ParseString(strLine, ' ', vWords);
    if (vWords.size() < 4)
        return false;

    string str = vWords[3];
    if (str.rfind("@") == string::npos)
        return false;
    string strHost = str.substr(str.rfind("@")+1);

    // Hybrid IRC used by lfnet always returns IP when you userhost yourself,
    // but in case another IRC is ever used this should work.
    printf("GetIPFromIRC() got userhost %s\n", strHost.c_str());
    CNetAddr addr(strHost, true);
    if (!addr.IsValid())
        return false;
    ipRet = addr;

    return true;
}

void ThreadIRCSeed3()
{

    try
    {
        ThreadIRCSeed2(NULL);
    }
    catch (std::exception& e) {
        fShutdown=true;
        //PrintExceptionContinue(&e, "ThreadIRCSeed3()");
    } catch (...) {
        fShutdown=true;
        //PrintExceptionContinue(NULL, "ThreadIRCSeed3()");
    }
    printf("ThreadIRCSeed exited\n");
}

void ThreadIRCSeed(void* parg)
{
    //IMPLEMENT_RANDOMIZE_STACK(ThreadIRCSeed(parg));

    // Make this thread recognisable as the IRC seeding thread
    RenameThread("bitcoin-ircseed");

    try
    {
        ThreadIRCSeed2(parg);
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "ThreadIRCSeed()");
    } catch (...) {
        PrintExceptionContinue(NULL, "ThreadIRCSeed()");
    }
    printf("ThreadIRCSeed exited\n");
}

void ThreadIRCSeed2(void* parg)
{
  /*
	// Dont advertise on IRC if we don't allow incoming connections *
    if (mapArgs.count("-connect") || fNoListen)
        return;

    if (GetBoolArg("-irc", false))
        return;
*/
    printf("ThreadIRCSeed started\n");
    int nErrorWait = 10;
    int nRetryWait = 10;

    loop
    {
        CService addrConnect("92.243.23.21", 6667); // irc.lfnet.org

        CService addrIRC("irc.lfnet.org", 6667, true);
        if (addrIRC.IsValid())
            addrConnect = addrIRC;

        SOCKET hSocket;
        if (!ConnectSocket(addrConnect, hSocket, 2000))
        {
            printf("IRC connect failed\n");
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }

        if (!RecvUntil(hSocket, "Found your hostname", "using your IP address instead", "Couldn't look up your hostname", "ignoring hostname"))
        {
            CloseSocket(hSocket);
            hSocket = INVALID_SOCKET;
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }

        CNetAddr addrIPv4("1.2.3.4"); // arbitrary IPv4 address to make GetLocal prefer IPv4 addresses
        CService addrLocal;
        string strMyName;
        if (GetLocal(addrLocal, &addrIPv4))
            strMyName = EncodeAddress(GetLocalAddress(&addrConnect));
        if (strMyName == "")
            strMyName = strprintf2("x%u", GetRand(1000000000));

        Send(hSocket, strprintf2("NICK %s\r", strMyName.c_str()).c_str());
        Send(hSocket, strprintf2("USER %s 8 * : %s\r", strMyName.c_str(), strMyName.c_str()).c_str());

        int nRet = RecvUntil(hSocket, " 004 ", " 433 ");
        if (nRet != 1)
        {
            CloseSocket(hSocket);
            hSocket = INVALID_SOCKET;
            if (nRet == 2)
            {
                printf("IRC name already in use\n");
                Wait(10);
                continue;
            }
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }
        Sleep2(500);

        // Get our external IP from the IRC server and re-nick before joining the channel
        CNetAddr addrFromIRC;
        if (GetIPFromIRC(hSocket, strMyName, addrFromIRC))
        {
            printf("GetIPFromIRC() returned %s\n", addrFromIRC.ToString().c_str());
            if (addrFromIRC.IsRoutable())
            {
                // IRC lets you to re-nick
                AddLocal(addrFromIRC, LOCAL_UPNP);
                strMyName = EncodeAddress(GetLocalAddress(&addrConnect));
                Send(hSocket, strprintf2("NICK %s\r", strMyName.c_str()).c_str());
            }
        }

		/*
        if (fTestNet) {
            Send(hSocket, "JOIN #fastcoinTEST3\r");
            Send(hSocket, "WHO #fastcoinTEST3\r");
        } else { */
            // randomly join #fastcoin00-#fastcoin99
            int channel_number = GetRandInt(100);
            channel_number = 0; // Fastcoin: for now, just use one channel
            Send(hSocket, strprintf2("JOIN #fastcoin%02d\r", channel_number).c_str());
            Send(hSocket, strprintf2("WHO #fastcoin%02d\r", channel_number).c_str());
       // }

        int64_t nStart = GetTime();
        string strLine;
        strLine.reserve(10000);
        while (RecvLineIRC(hSocket, strLine))
        {
            boost::this_thread::interruption_point();
            if (strLine.empty() || strLine.size() > 900 || strLine[0] != ':')
                continue;

            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords.size() < 2)
                continue;

            char pszName[10000];
            pszName[0] = '\0';

            if (vWords[1] == "352" && vWords.size() >= 8)
            {
                // index 7 is limited to 16 characters
                // could get full length name at index 10, but would be different from join messages
                strlcpy(pszName, vWords[7].c_str(), sizeof(pszName));
                printf("IRC got who\n");
            }

            if (vWords[1] == "JOIN" && vWords[0].size() > 1)
            {
                // :username!username@50000007.F000000B.90000002.IP JOIN :#channelname
                strlcpy(pszName, vWords[0].c_str() + 1, sizeof(pszName));
                if (strchr(pszName, '!'))
                    *strchr(pszName, '!') = '\0';
                printf("IRC got join\n");
            }

            if (pszName[0] == 'u')
            {
                CAddress addr;
                if (DecodeAddress(pszName, addr))
                {
                    addr.nTime = GetAdjustedTime();
                    //if (addrman.Add(addr, addrConnect, 51 * 60))
                    //    printf("IRC got new address: %s\n", addr.ToString().c_str());
                    nGotIRCAddresses++;
                }
                else
                {
                    printf("IRC decode failed\n");
                }
            }
        }
        CloseSocket(hSocket);
        hSocket = INVALID_SOCKET;

        if (GetTime() - nStart > 20 * 60)
        {
            nErrorWait /= 3;
            nRetryWait /= 3;
        }

        nRetryWait = nRetryWait * 11 / 10;
        if (!Wait(nRetryWait += 60))
            return;
    }
}










#ifdef TEST
int main(int argc, char *argv[])
{
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
        return false;
    }

    ThreadIRCSeed(NULL);

    WSACleanup();
    return 0;
}
#endif


