#include <fcntl.h>
#include <napi.h>
#include <unistd.h>

using namespace Napi;

#define PAGE_SIZE 4096
#define PAGE_METADATA_SIZE sizeof(uint16_t)

#define throwError(message) Error::New(info.Env(), message).ThrowAsJavaScriptException();

class IO : public Addon<IO> {
private:
    int fd;
    uint32_t entries, nextEntryOffset;

    void initMetadata() {
        lseek(fd, 0, SEEK_SET);
        entries = 0;
        write(fd, &entries, sizeof(entries));
        nextEntryOffset = PAGE_SIZE + PAGE_METADATA_SIZE;
        write(fd, &nextEntryOffset, sizeof(nextEntryOffset));
    }

    void cleanup() {
        lseek(fd, 0, SEEK_SET);
        write(fd, &entries, sizeof(entries));
        write(fd, &nextEntryOffset, sizeof(nextEntryOffset));
        close(fd);
    }

public:
    void init(const CallbackInfo &info) {
        info.Env().AddCleanupHook([this]() { cleanup(); });

        std::string path = info[0].As<String>().Utf8Value();
        fd = open(path.c_str(), O_RDWR);

        if (fd > 0) {
            if (lseek(fd, 0, SEEK_SET) == -1) throwError("Failed to seek");
            if (read(fd, &entries, sizeof(entries)) < 1) throwError("Failed to read entries");
            if (read(fd, &nextEntryOffset, sizeof(nextEntryOffset)) < 1) throwError("Failed to read nextEntryOffset");
            return;
        }
        if (errno != ENOENT) return throwError("Failed to open file");

        fd = open(path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == -1) return throwError("Failed to creat file");
        initMetadata();
    }

    Value getAll(const CallbackInfo &info) {
        Env env = info.Env();
        Array result = Array::New(env, entries);

        int i = 0;
        int pages = ceil((nextEntryOffset - PAGE_METADATA_SIZE) / (float) PAGE_SIZE) - 1;
        char buffer[PAGE_SIZE];
        if (lseek(fd, PAGE_SIZE, SEEK_SET) == -1) throwError("Failed to lseek");
        for (int page = 0; page < pages; page++) {
            if (read(fd, &buffer, PAGE_SIZE) < 1) throwError("Failed to read");
            uint16_t entries = *((uint16_t *) buffer);
            char *pointer = buffer + PAGE_METADATA_SIZE;
            while (entries-- > 0) {
                Object user = Object::New(env);
                user.Set("id", *(uint32_t *) pointer);
                pointer += sizeof(uint32_t);
                user.Set("age", *(uint8_t *) pointer);
                pointer += sizeof(uint8_t);
                user.Set("name", pointer);
                pointer += strlen(pointer) + 1;
                result.Set(i++, user);
            }
        }

        return result;
    }

    void append(const CallbackInfo &info) {
        Array arr = info[0].As<Array>();

        uint32_t page = floor(nextEntryOffset / PAGE_SIZE);
        uint16_t pageOffset = nextEntryOffset % PAGE_SIZE;
        uint16_t pageEntries = 0;
        if (pageOffset > PAGE_METADATA_SIZE)
            if (pread(fd, &pageEntries, sizeof(pageEntries), page * PAGE_SIZE) < 1) throwError("Failed to pread");

        if (lseek(fd, nextEntryOffset, SEEK_SET) == -1) throwError("Failed to lseek");
        for (size_t i = 0; i < arr.Length(); i++) {
            Object user = arr.Get(i).As<Object>();
            uint32_t id = user.Get("id").As<Number>().Int32Value();
            uint8_t age = user.Get("age").As<Number>().Int32Value();
            std::string name = user.Get("name").As<String>().Utf8Value();
            size_t entrySize = sizeof(id) + sizeof(age) + name.size() + 1;

            if (pageOffset + entrySize >= PAGE_SIZE) {
                if (pwrite(fd, &pageEntries, sizeof(pageEntries), page * PAGE_SIZE) < 1) throwError("Failed to pwrite");
                page++;
                if (lseek(fd, page * PAGE_SIZE, SEEK_SET) == -1) throwError("Failed to lseek");
                pageEntries = 0;
                if (write(fd, &pageEntries, sizeof(pageEntries)) < 1) throwError("Failed to write");
                pageOffset = PAGE_METADATA_SIZE;
            }

            if (write(fd, &id, sizeof(id)) < 1) throwError("Failed to write");
            if (write(fd, &age, sizeof(age)) < 1) throwError("Failed to write");
            if (write(fd, name.c_str(), name.size() + 1) < 1) throwError("Failed to write");

            pageEntries++;
            pageOffset += entrySize;
        }

        if (pwrite(fd, &pageEntries, sizeof(pageEntries), page * PAGE_SIZE) < 1) throwError("Failed to pwrite");
        entries += arr.Length();
        nextEntryOffset = page * PAGE_SIZE + pageOffset;
    }

    void reset(const CallbackInfo &info) {
        if (ftruncate(fd, 0) == -1) throwError("Couldn't reset storage!");
        initMetadata();
    }

    IO(Env env, Object exports) {
        DefineAddon(exports,
                    {InstanceMethod("init", &IO::init), InstanceMethod("getAll", &IO::getAll), InstanceMethod("append", &IO::append), InstanceMethod("reset", &IO::reset)});
    }
};

NODE_API_ADDON(IO)