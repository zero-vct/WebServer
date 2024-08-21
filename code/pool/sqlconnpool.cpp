#include "sqlconnpool.h"
using namespace std;

SqlConnPool::SqlConnPool() : useCount_(0), freeCount_(0), MAX_CONN_(0) {}

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char* host, int port, const char* user,
                       const char* pwd, const char* dbName, int connSize) {
    assert(connSize > 0);
    lock_guard<mutex> locker(mtx_);

    for (int i = 0; i < connSize; i++) {
        MYSQL *sql = mysql_init(nullptr);
        if (!sql) {
            LOG_ERROR("MySql init error!");
            continue;
        }
        sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!");
            mysql_close(sql);
            continue;
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connQue_.size();  // 只有成功的连接才会计入
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if(sem_wait(&semId_) != 0) {  // 添加检查
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    {
        lock_guard<mutex> locker(mtx_);
        if(connQue_.empty()) {
            LOG_WARN("No available connection!");
            sem_post(&semId_); // 信号量归还
            return nullptr;
        }
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* sql) {
    assert(sql);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId_);
}

void SqlConnPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);
    while (!connQue_.empty()) {
        MYSQL* sql = connQue_.front();
        connQue_.pop();
        mysql_close(sql);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}
