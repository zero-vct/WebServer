#include "httpconn.h"
#include <arpa/inet.h>  // For inet_ntoa
#include <cstring>      // For bzero
#include <unistd.h>     // For close
#include <cassert>      // For assert

using namespace std;

const char* HttpConn::srcDir = nullptr;
std::atomic<int> HttpConn::userCount(0);
bool HttpConn::isET = false;

HttpConn::HttpConn() 
    : fd_(-1), addr_({ 0 }), isClose_(true) {}

HttpConn::~HttpConn() { 
    Close(); 
}

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    fd_ = fd;
    addr_ = addr;
    isClose_ = false;
    ++userCount;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), static_cast<int>(userCount));
}

void HttpConn::Close() {
    if (!isClose_) {
        isClose_ = true;
        --userCount;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), static_cast<int>(userCount));
    }
}

 int HttpConn::GetFd() const {
    return fd_;
}

 struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

 const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

 int HttpConn::GetPort() const {
    return ntohs(addr_.sin_port);  // Convert port to host byte order
}

ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            if (len == 0) {
                LOG_INFO("Client disconnected");
            } else {
                LOG_ERROR("Read error: %s", strerror(errno));
            }
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0) {
            *saveErrno = errno;
            LOG_ERROR("Write error: %s", strerror(errno));
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0) { 
            break; 
        } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = static_cast<char*>(iov_[1].iov_base) + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        } else {
            iov_[0].iov_base = static_cast<char*>(iov_[0].iov_base) + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::process() {
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0) {
        return false;
    }
    
    if (request_.parse(readBuff_)) {
        LOG_DEBUG("Parsed request path: %s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        response_.Init(srcDir, request_.path(), false, 400);
    }

    response_.MakeResponse(writeBuff_);
    
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    if (response_.FileLen() > 0 && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }

    LOG_DEBUG("File size: %d, iovCnt: %d, ToWriteBytes: %d", response_.FileLen(), iovCnt_, ToWriteBytes());
    return true;
}
