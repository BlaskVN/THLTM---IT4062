#include <iostream>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>

void lookupByIP(const std::string& ip) {
    struct sockaddr_in sa;
    char host[1024];
    char service[20];

    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);

    int result = getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, sizeof(host), service, sizeof(service), NI_NAMEREQD);
    if (result != 0) {
        std::cout << "No information found" << std::endl;
        return;
    }

    std::cout << "Main name: " << host << std::endl;
    std::cout << "Alternate name: " << std::endl;
}

void lookupByDomain(const std::string& domain) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(domain.c_str(), NULL, &hints, &res)) != 0) {
        std::cout << "No information found" << std::endl;
        return;
    }

    std::cout << "Main IP: ";
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        std::string ipver;

        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        std::cout << ipstr << std::endl;
    }
    std::cout << "Alternate IP: " << std::endl;

    freeaddrinfo(res);
}

bool isValidIP(const std::string& ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Invalid option" << std::endl;
        return 1;
    }

    int option = std::stoi(argv[1]);
    std::string parameter = argv[2];

    if (option == 1) {
        if (isValidIP(parameter)) {
            lookupByIP(parameter);
        } else {
            std::cout << "Invalid option" << std::endl;
        }
    } else if (option == 2) {
        lookupByDomain(parameter);
    } else {
        std::cout << "Invalid option" << std::endl;
    }

    return 0;
}