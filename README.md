Fastcoin Core integration/staging tree
=====================================

https://fastcoin.org

What is Fastcoin?
----------------

Fastcoin is an experimental new digital currency that enables instant payments to
anyone, anywhere in the world. Fastcoin uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network. Fastcoin Core is the name of open source
software which enables the use of this currency.

For more information, as well as an immediately useable, binary version of
the Fastcoin Core software, see https://fastcoin.org

License
-------

The master branch is regularly built and tested, but is not guaranteed
to be completely stable. Tags are regularly created to indicate new
official, stable release versions of Fastcoin.

Fastcoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see http://opensource.org/licenses/MIT.


Development process
-------------------

Developers work in their own trees, then submit pull requests when they think
their feature or bug fix is ready.

If it is a simple/trivial/non-controversial change, then one of the Fastcoin
development team members simply pulls it.

If it is a *more complicated or potentially controversial* change, then the patch
submitter will be asked to start a discussion (if they haven't already) on the
[mailing list](https://groups.google.com/forum/#!forum/fastcoin-dev).

The patch will be accepted if there is broad consensus that it is a good thing.
Developers should expect to rework and resubmit patches if the code doesn't
match the project's coding conventions (see [doc/coding.md](doc/coding.md)) or are
controversial.

The `master-0.10` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/fastcoin-project/fastcoin/tags) are created
regularly to indicate new official, stable release versions of Fastcoin.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Manual Quality Assurance (QA) Testing

Large changes should have a test plan, and should be tested by somebody other
than the developer who wrote the code.
See https://github.com/bitcoin/QA/ for how to create a test plan.

* https://twitter.com/fast_coin
* https://twitter.com/FSTFoundation
* https://twitter.com/JonMarshallz
* https://www.facebook.com/pages/Fastcoinca/593923330628082
* https://webchat.freenode.net/?channels=#fastcoin

=======

Translations
------------


**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

* [Cryptopia](https://www.cryptopia.co.nz/Exchange?market=FST_BTC)
* [LiveCoin](https://www.livecoin.net/)

