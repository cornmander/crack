
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "debug/DebugTools.h"
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"
#include "Dir.h"
#include "Util.h"
#include "Net.h"
#include "Math.h"
#include "Exceptions.h"
#include "Process.h"
using namespace crack::ext;

extern "C" bool __CrackUncaughtException();
extern "C" void crack_runtime_time_init(crack::ext::Module *mod);

extern "C" void crack_runtime_init(Module *mod) {
    Type *byteptrType = mod->getByteptrType();
    Type *boolType = mod->getBoolType();
    Type *intType = mod->getIntType();
    Type *type_intz = mod->getIntzType();
    Type *type_uintz = mod->getUintzType();
    Type *uint32Type = mod->getUint32Type();
    Type *int32Type = mod->getInt32Type();
    Type *uintType = mod->getUintType();
    Type *byteType = mod->getByteType();
    Type *voidType = mod->getVoidType();
    Type *voidptrType = mod->getVoidptrType();

    Type *baseArrayType = mod->getType("array");
    // array[byteptr]
    Type *byteptrArrayType;
    {
        std::vector<Type *> params(1);
        params[0] = byteptrType;
        byteptrArrayType = baseArrayType->getSpecialization(params);
    }
    byteptrArrayType->finish();

    Type *cdentType = mod->addType("DirEntry", 
                                   sizeof(crack::runtime::DirEntry)
                                   );
    cdentType->addInstVar(byteptrType, "name", 
                          CRACK_OFFSET(crack::runtime::DirEntry, name)
                          );
    cdentType->addInstVar(intType, "type", 
                          CRACK_OFFSET(crack::runtime::DirEntry, type)
                          );
    cdentType->finish();

    Type *cdType = mod->addType("Dir", sizeof(crack::runtime::Dir));
    // XXX should these be part of addType?
    cdType->addMethod(voidptrType, "oper to .builtin.voidptr", 
                      (void *)crack::runtime::Dir_toVoidptr
                      );
    cdType->addMethod(boolType, "oper to .builtin.bool", 
                      (void *)crack::runtime::Dir_toBool
                      );
    cdType->finish();
    
    Func *f = mod->addFunc(cdType, "opendir", (void *)crack::runtime::opendir);
    f->addArg(byteptrType, "name");
    
    f = mod->addFunc(cdentType, "getDirEntry", 
                     (void *)crack::runtime::getDirEntry
                     );
    f->addArg(cdType, "d");
    
    f = mod->addFunc(intType, "closedir", (void *)crack::runtime::closedir);
    f->addArg(cdType, "d");
    
    f = mod->addFunc(intType, "readdir", (void *)crack::runtime::readdir);
    f->addArg(cdType, "d");
    
    f = mod->addFunc(intType, "fnmatch", (void *)crack::runtime::fnmatch);
    f->addArg(byteptrType, "pattern");
    f->addArg(byteptrType, "string");
    
    f = mod->addFunc(byteptrType, "basename", (void *)basename, "basename");
    f->addArg(byteptrType, "path");
    
    f = mod->addFunc(byteptrType, "dirname", (void *)dirname, "dirname");
    f->addArg(byteptrType, "path");

    f = mod->addFunc(intType, "is_file", (void *)crack::runtime::is_file);
    f->addArg(byteptrType, "path");
    
    f = mod->addFunc(boolType, "fileExists",
                     (void *)crack::runtime::fileExists
                     );
    f->addArg(byteptrType, "path");
    
    Type *statType = mod->addType("Stat", sizeof(struct stat));
    statType->addInstVar(intType, "st_dev", CRACK_OFFSET(struct stat, st_dev));
    statType->addInstVar(intType, "st_ino", CRACK_OFFSET(struct stat, st_ino));
    statType->addInstVar(intType, "st_mode", CRACK_OFFSET(struct stat, st_mode));
    statType->addInstVar(intType, "st_nlink", 
                         CRACK_OFFSET(struct stat, st_nlink)
                         );
    statType->addInstVar(intType, "st_uid", CRACK_OFFSET(struct stat, st_uid));
    statType->addInstVar(intType, "st_gid", CRACK_OFFSET(struct stat, st_gid));
    statType->addInstVar(intType, "st_rdev", CRACK_OFFSET(struct stat, st_rdev));
    statType->addInstVar(intType, "st_size", CRACK_OFFSET(struct stat, st_size));
    statType->addInstVar(intType, "st_blksize", 
                         CRACK_OFFSET(struct stat, st_blksize)
                         );
    statType->addInstVar(intType, "st_blocks", 
                         CRACK_OFFSET(struct stat, st_blocks)
                         );
    statType->addInstVar(intType, "st_atime", 
                         CRACK_OFFSET(struct stat, st_atime)
                         );
    statType->addInstVar(intType, "st_mtime", 
                         CRACK_OFFSET(struct stat, st_mtime)
                         );
    statType->addInstVar(intType, "st_ctime", 
                         CRACK_OFFSET(struct stat, st_ctime)
                         );
    statType->addConstructor();
    statType->finish();

    f = mod->addFunc(intType, "stat", (void *)stat, "stat");
    f->addArg(byteptrType, "path");
    f->addArg(statType, "buf");

    f = mod->addFunc(intType, "fileRemove", (void *)remove);
    f->addArg(byteptrType, "path");

    f = mod->addFunc(intType, "mkdir", (void *)mkdir);
    f->addArg(byteptrType, "path");
    f->addArg(intType, "mode");

    f = mod->addFunc(byteptrType, "c_strerror",
                     (void *)crack::runtime::strerror
                     );
    
    f = mod->addFunc(mod->getVoidType(), "float_str", 
                     (void *)crack::runtime::float_str
                     );
    f->addArg(mod->getFloat64Type(), "val");
    f->addArg(byteptrType, "buf");
    f->addArg(mod->getUintType(), "size");

    f = mod->addFunc(uintType, "rand", 
                     (void *)crack::runtime::rand
                     );
    f->addArg(mod->getUintType(), "low");
    f->addArg(mod->getUintType(), "high");
    
    f = mod->addFunc(intType, "random", (void *)random);

    f = mod->addFunc(voidType, "srandom", (void *)srandom);
    f->addArg(uintType, "seed");

    f = mod->addFunc(byteptrType, "puts",
                     (void *)crack::runtime::puts
                     );
    f->addArg(mod->getByteptrType(), "str");

    f = mod->addFunc(byteptrType, "__die",
                     (void *)crack::runtime::__die
                     );
    f->addArg(mod->getByteptrType(), "message");

    f = mod->addFunc(byteptrType, "printfloat",
                     (void *)crack::runtime::printfloat
                     );
    f->addArg(mod->getFloatType(), "val");

    f = mod->addFunc(byteptrType, "printint",
                     (void *)crack::runtime::printint
                     );
    f->addArg(mod->getIntType(), "val");

    f = mod->addFunc(byteptrType, "printint64",
                     (void *)crack::runtime::printint
                     );
    f->addArg(mod->getInt64Type(), "val");

    f = mod->addFunc(byteptrType, "printuint64",
                     (void *)crack::runtime::printint
                     );
    f->addArg(mod->getUint64Type(), "val");

    // normal file open and close.

    f = mod->addFunc(intType, "open", (void *)open);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "mode");

    f = mod->addFunc(intType, "open", (void *)open);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "flags");
    f->addArg(intType, "mode");

    f = mod->addFunc(intType, "creat", (void *)open);
    f->addArg(byteptrType, "pathname");
    f->addArg(intType, "mode");

    mod->addConstant(intType, "O_RDONLY", O_RDONLY);
    mod->addConstant(intType, "O_WRONLY", O_WRONLY);
    mod->addConstant(intType, "O_RDWR", O_RDWR);
    mod->addConstant(intType, "O_APPEND", O_APPEND);
    mod->addConstant(intType, "O_ASYNC", O_ASYNC);
    mod->addConstant(intType, "O_CREAT", O_CREAT);

    // Net
    
    mod->addConstant(intType, "AF_UNIX", AF_UNIX);
    mod->addConstant(intType, "AF_LOCAL", AF_LOCAL);
    mod->addConstant(intType, "AF_INET", AF_INET);
    mod->addConstant(intType, "AF_INET6", AF_INET6);
    mod->addConstant(intType, "AF_IPX", AF_IPX);
    mod->addConstant(intType, "AF_NETLINK", AF_NETLINK);
    mod->addConstant(intType, "AF_X25", AF_X25);
    mod->addConstant(intType, "AF_AX25", AF_AX25);
    mod->addConstant(intType, "AF_ATMPVC", AF_ATMPVC);
    mod->addConstant(intType, "AF_APPLETALK", AF_APPLETALK);
    mod->addConstant(intType, "AF_PACKET", AF_PACKET);
    mod->addConstant(intType, "SOCK_STREAM", SOCK_STREAM);
    mod->addConstant(intType, "SOCK_DGRAM", SOCK_DGRAM);
    mod->addConstant(intType, "SOCK_SEQPACKET", SOCK_SEQPACKET);
    mod->addConstant(intType, "SOCK_RAW", SOCK_RAW);
    mod->addConstant(intType, "SOCK_RDM", SOCK_RDM);
    mod->addConstant(intType, "SOCK_PACKET", SOCK_PACKET);
    mod->addConstant(intType, "SOCK_NONBLOCK", SOCK_NONBLOCK);
    mod->addConstant(intType, "SOCK_CLOEXEC", SOCK_CLOEXEC);
    mod->addConstant(intType, "SOL_SOCKET", SOL_SOCKET);
    mod->addConstant(intType, "SO_REUSEADDR", SO_REUSEADDR);
    mod->addConstant(intType, "POLLIN", POLLIN);
    mod->addConstant(intType, "POLLOUT", POLLOUT);
    mod->addConstant(intType, "POLLPRI", POLLPRI);
    mod->addConstant(intType, "POLLERR", POLLERR);
    mod->addConstant(intType, "POLLHUP", POLLHUP);
    mod->addConstant(intType, "POLLNVAL", POLLNVAL);
    mod->addConstant(uint32Type, "INADDR_ANY", static_cast<int>(INADDR_ANY));
    
    f = mod->addFunc(uint32Type, "makeIPV4", 
                     (void*)crack::runtime::makeIPV4);
    f->addArg(byteType, "a");
    f->addArg(byteType, "b");
    f->addArg(byteType, "c");
    f->addArg(byteType, "d");

    // begin SockAddr
    Type *sockAddrType = mod->addType("SockAddr", sizeof(sockaddr));
    sockAddrType->addConstructor();
    sockAddrType->addInstVar(intType, "family", 
                             CRACK_OFFSET(sockaddr, sa_family)
                             );
    sockAddrType->finish();
    // end SockAddr

    // begin SockAddrIn    
    Type *sockAddrInType = mod->addType("SockAddrIn", 
                                        sizeof(sockaddr_in) - sizeof(sockaddr)
                                        );
    sockAddrInType->addBase(sockAddrType);
    sockAddrInType->addInstVar(uint32Type, "addr", 
                               CRACK_OFFSET(sockaddr_in, sin_addr.s_addr)
                               );
    sockAddrInType->addInstVar(uintType, "port",
                               CRACK_OFFSET(sockaddr_in, sin_port)
                               );

    f = sockAddrInType->addConstructor(
        "init",
        (void *)&crack::runtime::SockAddrIn::init1
    );
    f->addArg(byteType, "a");
    f->addArg(byteType, "b");
    f->addArg(byteType, "c");
    f->addArg(byteType, "d");
    f->addArg(intType, "port");

    f = sockAddrInType->addConstructor(
        "init",
        (void *)&crack::runtime::SockAddrIn::init2
    );
    f->addArg(uint32Type, "addr");
    f->addArg(uintType, "port");
    
    sockAddrInType->finish();
    // end SockAddrIn
    
    f = mod->addFunc(intType, "connect", (void *)crack::runtime::connect);
    f->addArg(intType, "s");
    f->addArg(sockAddrType, "addr");
    
    f = mod->addFunc(intType, "bind",
                     (void *)crack::runtime::bind
                     );
    f->addArg(intType, "s");
    f->addArg(sockAddrType, "addr");
    
    f = mod->addFunc(intType, "accept",
                     (void *)crack::runtime::accept
                     );
    f->addArg(intType, "s");
    f->addArg(sockAddrType, "addr");
    
    f = mod->addFunc(intType, "setsockopt",
                     (void *)crack::runtime::setsockopt_int
                     );
    f->addArg(intType, "s");
    f->addArg(intType, "level");
    f->addArg(intType, "optname");
    f->addArg(intType, "val");

    // begin PollEvt
    Type *pollEventType = mod->addType("PollEvt", 
                                       sizeof(crack::runtime::PollEvt));
    pollEventType->addInstVar(intType, "fd", 
                              CRACK_OFFSET(crack::runtime::PollEvt, fd)
                              );
    pollEventType->addInstVar(intType, "events", 
                              CRACK_OFFSET(crack::runtime::PollEvt, events)
                              );
    pollEventType->addInstVar(intType, "revents", 
                              CRACK_OFFSET(pollfd, revents)
                              );
    pollEventType->addConstructor();
    pollEventType->finish();
    // end PollEvent

    // begin TimeVal
    Type *timeValType = mod->addType("TimeVal", 
                                     sizeof(crack::runtime::TimeVal)
                                     );
    timeValType->addInstVar(int32Type, "secs", 
                            CRACK_OFFSET(crack::runtime::TimeVal, secs)
                            );
    timeValType->addInstVar(int32Type, "nsecs", 
                            CRACK_OFFSET(crack::runtime::TimeVal, nsecs)
                            );

    f = timeValType->addConstructor("init", 
                                    (void *)&crack::runtime::TimeVal::init
                                    );
    f->addArg(int32Type, "secs");
    f->addArg(int32Type, "nsecs");
    
    f = timeValType->addMethod(voidType, "setToNow",
                               (void *)gettimeofday
                               );
    f->addArg(voidptrType, "tz");

    timeValType->finish();
    // end TimeVal
    
    // begin SigSet
    Type *sigSetType = mod->addType("SigSet", 0);
    f = sigSetType->addStaticMethod(sigSetType, "oper new",
                                    (void *)crack::runtime::SigSet_create
                                    );
    
    f = sigSetType->addMethod(voidType, "destroy",
                              (void *)crack::runtime::SigSet_destroy
                              );

    f = sigSetType->addMethod(voidType, "empty",
                              (void *)crack::runtime::SigSet_empty
                              );
    
    f = sigSetType->addMethod(voidType, "fill",
                              (void *)crack::runtime::SigSet_fill
                              );

    f = sigSetType->addMethod(voidType, "add",
                              (void *)crack::runtime::SigSet_add
                              );
    f->addArg(intType, "signum");

    f = sigSetType->addMethod(voidType, "del",
                              (void *)crack::runtime::SigSet_del
                              );
    f->addArg(intType, "signum");

    f = sigSetType->addMethod(voidType, "has",
                              (void *)crack::runtime::SigSet_has
                              );
    f->addArg(intType, "signum");

    sigSetType->finish();
    // end SigSet

    // begin PollSet
    Type *pollSetType = mod->addType("PollSet", 0);
    f = pollSetType->addStaticMethod(pollSetType, "oper new",
                                     (void *)&crack::runtime::PollSet_create
                                     );
    f->addArg(uintType, "size");
    
    f = pollSetType->addMethod(voidType, "copy",
                               (void *)crack::runtime::PollSet_copy
                               );
    f->addArg(pollSetType, "src");
    f->addArg(uintType, "size");
    
    f = pollSetType->addMethod(voidType, "destroy",
                               (void *)crack::runtime::PollSet_destroy
                               );
    f = pollSetType->addMethod(voidType, "set",
                               (void *)crack::runtime::PollSet_set
                               );
    f->addArg(uintType, "index");
    f->addArg(intType, "fd");
    f->addArg(intType, "events");
    f->addArg(intType, "revents");

    f = pollSetType->addMethod(voidType, "get",
                               (void *)crack::runtime::PollSet_get
                               );
    f->addArg(uintType, "index");
    f->addArg(pollEventType, "event");

    f = pollSetType->addMethod(intType, "next",
                               (void *)crack::runtime::PollSet_next
                               );
    f->addArg(uintType, "size");
    f->addArg(uintType, "index");
    f->addArg(pollEventType, "outputEntry");
    
    f = pollSetType->addMethod(intType, "poll",
                               (void *)crack::runtime::PollSet_poll
                               );
    f->addArg(uintType, "nfds");
    f->addArg(timeValType, "tv");
    f->addArg(sigSetType, "sigmask");

    pollSetType->finish();
    // end PollSet

    // misc C functions
    f = mod->addFunc(intType, "close", (void *)close, "close");
    f->addArg(intType, "fd");
    
    f = mod->addFunc(intType, "socket", (void *)socket, "socket");
    f->addArg(intType, "domain");
    f->addArg(intType, "type");
    f->addArg(intType, "protocol");
    
    f = mod->addFunc(intType, "listen", (void *)listen, "listen");
    f->addArg(intType, "fd");
    f->addArg(intType, "backlog");
    
    f = mod->addFunc(intType, "send", (void *)send, "send");
    f->addArg(intType, "fd");
    f->addArg(byteptrType, "buf");
    f->addArg(uintType, "size");
    f->addArg(intType, "flags");
    
    f = mod->addFunc(intType, "recv", (void *)recv, "recv");
    f->addArg(intType, "fd");
    f->addArg(byteptrType, "buf");
    f->addArg(uintType, "size");
    f->addArg(intType, "flags");

    f = mod->addFunc(voidType, "abort", (void *)abort, "abort");

    f = mod->addFunc(voidType, "free", (void *)free, "free");
    f->addArg(voidptrType, "size");
    
    f = mod->addFunc(voidType, "strcpy", (void *)strcpy, "strcpy");
    f->addArg(byteptrType, "dst");
    f->addArg(byteptrType, "src");

    f = mod->addFunc(uintType, "strlen", (void *)strlen, "strlen");
    f->addArg(byteptrType, "str");

    f = mod->addFunc(byteptrType, "malloc", (void *)malloc, "malloc");
    f->addArg(uintType, "size");
    
    f = mod->addFunc(byteptrType, "memcpy", (void *)memcpy, "memcpy");
    f->addArg(byteptrType, "dst");
    f->addArg(byteptrType, "src");
    f->addArg(uintType, "size");

    f = mod->addFunc(byteptrType, "memset", (void *)memset, "memset");
    f->addArg(byteptrType, "dst");
    f->addArg(byteType, "c");
    f->addArg(uintType, "size");

    f = mod->addFunc(intType, "memcmp", (void *)memcmp, "memcmp");
    f->addArg(byteptrType, "a");
    f->addArg(byteptrType, "b");
    f->addArg(uintType, "size");
    
    f = mod->addFunc(byteptrType, "memmove", (void *)memmove, "memmove");
    f->addArg(byteptrType, "dst");
    f->addArg(byteptrType, "src");
    f->addArg(uintType, "size");
    
    Type *cFileType = mod->addType("CFile", 0);
    cFileType->finish();
    
    f = mod->addFunc(cFileType, "fopen", (void *)fopen, "fopen");
    f->addArg(byteptrType, "path");
    f->addArg(byteptrType, "mode");
    
    f = mod->addFunc(intType, "fclose", (void *)fclose, "close");
    f->addArg(cFileType, "fp");
    
    f = mod->addFunc(intType, "fileno", (void *)fileno, "fileno");
    f->addArg(cFileType, "fp");

    f = mod->addFunc(intType, "read", (void *)read, "read");
    f->addArg(intType, "fd");
    f->addArg(byteptrType, "buf");
    f->addArg(uintType, "count");

    f = mod->addFunc(intType, "write", (void *)write, "write");
    f->addArg(intType, "fd");
    f->addArg(byteptrType, "buf");
    f->addArg(uintType, "count");

    f = mod->addFunc(voidType, "exit", (void *)exit, "exit");
    f->addArg(intType, "status");

    // Add math functions
    crack::runtime::math_init(mod);
    
    // Add time functions
    crack_runtime_time_init(mod);
    
    // add exception functions
    mod->addConstant(intType, "EXCEPTION_MATCH_FUNC", 
                     crack::runtime::exceptionMatchFuncHook
                     );
    mod->addConstant(intType, "BAD_CAST_FUNC",
                     crack::runtime::badCastFuncHook
                     );
    mod->addConstant(intType, "EXCEPTION_RELEASE_FUNC",
                     crack::runtime::exceptionReleaseFuncHook
                     );
    mod->addConstant(intType, "EXCEPTION_PERSONALITY_FUNC",
                     crack::runtime::exceptionPersonalityFuncHook
                     );
    mod->addConstant(intType, "EXCEPTION_FRAME_FUNC",
                     crack::runtime::exceptionFrameFuncHook
                     );
    mod->addConstant(intType, "EXCEPTION_UNCAUGHT_FUNC",
                     crack::runtime::exceptionUncaughtFuncHook
                     );
    f = mod->addFunc(voidType, "registerHook", 
                     (void *)crack::runtime::registerHook
                     );
    f->addArg(intType, "hookId");
    f->addArg(voidptrType, "hook");
    
    // This shouldn't need to be registered, but as it stands, runtime just 
    // gets loaded like any other module in JIT mode and this is resolved at 
    // runtime.
    mod->addFunc(mod->getBoolType(), "__CrackUncaughtException",
                 (void *)__CrackUncaughtException
                 );

    // Process support
    mod->addConstant(intType, "SIGABRT", SIGABRT);
    mod->addConstant(intType, "SIGALRM", SIGALRM);
    mod->addConstant(intType, "SIGBUS" , SIGBUS);
    mod->addConstant(intType, "SIGCHLD", SIGCHLD);
    mod->addConstant(intType, "SIGCLD" , SIGCLD);
    mod->addConstant(intType, "SIGCONT", SIGCONT);
    mod->addConstant(intType, "SIGFPE" , SIGFPE);
    mod->addConstant(intType, "SIGHUP" , SIGHUP);
    mod->addConstant(intType, "SIGILL" , SIGILL);
    mod->addConstant(intType, "SIGINT" , SIGINT);
    mod->addConstant(intType, "SIGIO"  , SIGIO);
    mod->addConstant(intType, "SIGIOT" , SIGIOT);
    mod->addConstant(intType, "SIGKILL", SIGKILL);
    mod->addConstant(intType, "SIGPIPE", SIGPIPE);
    mod->addConstant(intType, "SIGPOLL", SIGPOLL);
    mod->addConstant(intType, "SIGPROF", SIGPROF);
    mod->addConstant(intType, "SIGPWF" , SIGPWR);
    mod->addConstant(intType, "SIGQUIT", SIGQUIT);
    mod->addConstant(intType, "SIGSEGV", SIGSEGV);
    mod->addConstant(intType, "SIGSTOP", SIGSTOP);
    mod->addConstant(intType, "SIGSYS" , SIGSYS);
    mod->addConstant(intType, "SIGTERM", SIGTERM);
    mod->addConstant(intType, "SIGTRAP", SIGTRAP);
    mod->addConstant(intType, "SIGTSTP", SIGTSTP);
    mod->addConstant(intType, "SIGTTIN", SIGTTIN);
    mod->addConstant(intType, "SIGURG" , SIGURG);
    mod->addConstant(intType, "SIGUSR1", SIGUSR1);
    mod->addConstant(intType, "SIGUSR2", SIGUSR2);
    mod->addConstant(intType, "SIGVTALRM", SIGVTALRM);
    mod->addConstant(intType, "SIGWINCH" , SIGWINCH);
    mod->addConstant(intType, "SIGXCPU", SIGXCPU);
    mod->addConstant(intType, "SIGXFSZ", SIGXFSZ);

    Type *cpipeType = mod->addType("PipeDesc", 
                                   sizeof(crack::runtime::PipeDesc)
                                   );
    cpipeType->addInstVar(intType, "flags", 
                          CRACK_OFFSET(crack::runtime::PipeDesc, flags)
                          );
    cpipeType->addInstVar(intType, "stdin", 
                          CRACK_OFFSET(crack::runtime::PipeDesc, stdin)
                          );
    cpipeType->addInstVar(intType, "stdout", 
                          CRACK_OFFSET(crack::runtime::PipeDesc, stdout)
                          );
    cpipeType->addInstVar(intType, "stderr", 
                          CRACK_OFFSET(crack::runtime::PipeDesc, stderr)
                          );
    cpipeType->addConstructor();
    cpipeType->finish();

    f = mod->addFunc(intType, "runChildProcess",
                     (void *)&crack::runtime::runChildProcess);
    f->addArg(byteptrArrayType, "argv");
    f->addArg(byteptrArrayType, "env");
    f->addArg(cpipeType, "pipes");

    f = mod->addFunc(intType, "closeProcess",
                     (void *)&crack::runtime::closeProcess);
    f->addArg(cpipeType, "pipes");

    f = mod->addFunc(intType, "waitProcess",
                     (void *)&crack::runtime::waitProcess);
    f->addArg(intType, "pid");
    f->addArg(intType, "noHang");

    f = mod->addFunc(voidType, "signalProcess",
                     (void *)&crack::runtime::signalProcess);
    f->addArg(intType, "pid");
    f->addArg(intType, "sig");

    // debug support
    f = mod->addFunc(voidType, "getLocation", 
                     (void *)&crack::debug::getLocation
                     );
    f->addArg(voidptrType, "address");
    f->addArg(byteptrArrayType, "info");
}
