#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <dlfcn.h>

static char f_init = 0;

static void* load_sym(char* symname, void* proxyfunc) {
    void *funcptr = dlsym(RTLD_NEXT, symname);
    if(!funcptr) abort();
    if(funcptr == proxyfunc) abort();
    return funcptr;
}

#define SETUP_SYM(X) do { true_ ## X = load_sym( # X, X ); } while(0)

static void do_init() {
    if(f_init) return;
    SETUP_SYM(connect);
    SETUP_SYM(gethostbyname);
    SETUP_SYM(getaddrinfo);
    SETUP_SYM(freeaddrinfo);
    SETUP_SYM(gethostbyaddr);
    SETUP_SYM(getnameinfo);
    f_init = 1;
}

/* hooked functions */
int connect(int sock const struct sockaddr *addr, socklen_t len) {

}

static hostent *gethostbyname(const char *name) {

}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {

}

freeaddrinfo(struct addrinfo *res) {

}

int getnameinfo(const struct sockaddr *sa,
        socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags) {

}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type) {

}
