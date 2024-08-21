#include "epoller.h"
#include <stdexcept>
#define DEFAULT_EPOLL_SIZE 512

Epoller::Epoller(int maxEvent)
    : epollFd_(epoll_create(DEFAULT_EPOLL_SIZE)), events_(maxEvent) {
    if (epollFd_ < 0 || events_.size() <= 0) {
        throw std::runtime_error("Failed to create epoll file descriptor or invalid maxEvent size");
    }
}

Epoller::~Epoller() {
    if (epollFd_ >= 0) {
        close(epollFd_);
    }
}

bool Epoller::AddFd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event ev = {};
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoller::ModFd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event ev = {};
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool Epoller::DelFd(int fd) {
    if (fd < 0) return false;
    epoll_event ev = {};
    return epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev) == 0;
}

int Epoller::Wait(int timeoutMs) {
    return epoll_wait(epollFd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::GetEventFd(size_t i) const {
    assert(i < events_.size());
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
    assert(i < events_.size());
    return events_[i].events;
}
