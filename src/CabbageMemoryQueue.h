#pragma once
#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

namespace cabbage {

class SharedMemoryQueue
{
public:
    SharedMemoryQueue(const std::string& shmName, size_t queueSize, size_t blockSize)
        : shmName(shmName), queueSize(queueSize), blockSize(blockSize)
    {
        // Try to open the shared memory object (check if it already exists)
        shmFd = shm_open(shmName.c_str(), O_RDWR, 0666);
        bool isNew = false;

        if (shmFd == -1)
        {
            // Shared memory does not exist, create it
            shmFd = shm_open(shmName.c_str(), O_CREAT | O_RDWR, 0666);
            if (shmFd == -1)
            {
                throw std::runtime_error("Failed to create shared memory object");
            }
            isNew = true;
        }

        // Set or verify the size of the shared memory object
        size_t totalSize = 2 * (sizeof(SharedMemoryHeader) + queueSize * blockSize);
        if (isNew)
        {
            if (ftruncate(shmFd, totalSize) == -1)
            {
                throw std::runtime_error("Failed to set size of shared memory object");
            }
        }

        // Map the shared memory object into the address space
        shmPtr = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
        if (shmPtr == MAP_FAILED)
        {
            throw std::runtime_error("Failed to map shared memory object");
        }

        // Initialize or attach to the queues
        childToHostHeader = reinterpret_cast<SharedMemoryHeader*>(shmPtr);
        hostToChildHeader = reinterpret_cast<SharedMemoryHeader*>(reinterpret_cast<char*>(shmPtr) + sizeof(SharedMemoryHeader) + queueSize * blockSize);

        if (isNew)
        {
            // Initialize the queue headers if this is the first process
            childToHostHeader->head.store(0);
            childToHostHeader->tail.store(0);
            childToHostHeader->size.store(queueSize);
            childToHostHeader->refCount.store(1);  // Initialize refCount

            hostToChildHeader->head.store(0);
            hostToChildHeader->tail.store(0);
            hostToChildHeader->size.store(queueSize);
            hostToChildHeader->refCount.store(1);  // Initialize refCount
        }
        else
        {
            // Attach to existing shared memory and increment refCount
            childToHostHeader->refCount.fetch_add(1);
            hostToChildHeader->refCount.fetch_add(1);
        }
    }

    ~SharedMemoryQueue()
    {
        // Decrement the reference counter - last instance frees memory
        if (childToHostHeader->refCount.fetch_sub(1) == 1)
        {
            // Unmap the shared memory object
            size_t totalSize = 2 * (sizeof(SharedMemoryHeader) + queueSize * blockSize);
            munmap(shmPtr, totalSize);

            // Close the shared memory object
            close(shmFd);

            // Remove the shared memory object
            shm_unlink(shmName.c_str());

            // Debug: Log that the shared memory is being freed
            std::cout << "Shared memory freed (Process ID: " << getpid() << ")" << std::endl;
        }
        else
        {
            // Debug: Log that the process is detaching
            std::cout << "Process detached from shared memory (Process ID: " << getpid() << ")" << std::endl;
        }
    }

    bool sendToChild(const nlohmann::json& obj)
    {
        return push(hostToChildHeader, obj);
    }

    bool receiveFromChild(nlohmann::json& obj)
    {
        return pop(childToHostHeader, obj);
    }

    bool sendToHost(const nlohmann::json& obj)
    {
        return push(childToHostHeader, obj);
    }

    bool receiveFromHost(nlohmann::json& obj)
    {
        return pop(hostToChildHeader, obj);
    }

private:
    struct SharedMemoryHeader
    {
        std::atomic<size_t> head;   // Index of the next element to be written
        std::atomic<size_t> tail;   // Index of the next element to be read
        std::atomic<size_t> size;   // Maximum number of elements in the queue
        std::atomic<int> refCount;  // Reference counter
    };

    bool push(SharedMemoryHeader* header, const nlohmann::json& obj)
    {
        size_t head = header->head.load();
        size_t nextHead = (head + 1) % header->size.load();

        if (nextHead == header->tail.load())
        {
            return false; // Queue is full
        }

        // Serialize the JSON object to a fixed-size block
        std::string serialized = obj.dump();
        if (serialized.size() >= blockSize)
        {
            throw std::runtime_error("JSON object too large for block size");
        }

        // Copy the serialized data to the shared memory
        char* queueBase = reinterpret_cast<char*>(shmPtr) + sizeof(SharedMemoryHeader);
        std::memcpy(queueBase + head * blockSize, serialized.c_str(), serialized.size());

        // Null-terminate the message
        queueBase[head * blockSize + serialized.size()] = '\0';

        // Update the head
        header->head.store(nextHead);

        return true;
    }

    bool pop(SharedMemoryHeader* header, nlohmann::json& obj)
    {
        size_t tail = header->tail.load();

        if (tail == header->head.load())
        {
            return false; // Queue is empty
        }

        // Copy the serialized data from the shared memory
        char* queueBase = reinterpret_cast<char*>(shmPtr) + sizeof(SharedMemoryHeader);
        char* block = queueBase + tail * blockSize;

        // Find the length of the JSON message (up to the first null terminator)
        size_t messageLength = std::strlen(block);

        // Construct the string using the actual message length
        std::string serialized(block, messageLength);

        // Deserialize the JSON object
        obj = nlohmann::json::parse(serialized);

        // Update the tail
        header->tail.store((tail + 1) % header->size.load());

        return true;
    }

    std::string shmName;
    size_t queueSize;
    size_t blockSize;
    int shmFd;
    void* shmPtr;
    SharedMemoryHeader* childToHostHeader;
    SharedMemoryHeader* hostToChildHeader;
};

} // namespace cabbage
