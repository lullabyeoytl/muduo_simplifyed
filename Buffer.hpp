#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>

/// +---------------------+-----------------+----------------+
/// |  prependable bytes  |  readable bytes  |  writable bytes  |
/// |                     |     (CONTENT)    |                  |
/// |                     |                 |                  |
/// +---------------------+-----------------+----------------+
/// 0                   8                 8+readable bytes   size
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {}

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writeableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    void retrieve(size_t len){
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else{
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWritableBytes(size_t len)
    {
        if (writeableBytes() < len) 
        {
            makeSpace(len);
        }
    }

    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len , beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    ssize_t readFd(int fd, int* saveErrno);
private:
    // 兼容纯c接口
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
        if (writeableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        }else{
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readable + readerIndex_;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};