import { Writable, Readable } from 'stream';
import bind from 'bindings';
const io = bind('io');

export type User = {
    id: number;
    name: string;
    age: number;
};

type Options = { highWaterMark: number };

class UserReader extends Readable {
    private page: number = 0;

    constructor({ highWaterMark }: Options) {
        super({ highWaterMark, objectMode: true });
    }

    _read = (size: number) => {
        this.push(io.getPages(this.page, size));
        this.page += size;
    };
}

class UserWriter extends Writable {
    private buffer: User[] = [];

    constructor({ highWaterMark }: Options) {
        super({ highWaterMark, objectMode: true });
    }

    _write = (data: User, _: any, callback: (error?: Error) => void): void => {
        try {
            if (this.buffer.length < this.writableHighWaterMark) {
                this.buffer.push(data);
            } else {
                io.append(this.buffer);
                this.buffer = [];
            }
            callback();
        } catch (e: any) {
            console.error(e);
            callback(e);
        }
    };

    _final = (callback: (error?: Error) => void): void => {
        io.append(this.buffer);
        this.buffer = [];
        callback();
    };
}

class UserStorage {
    constructor(path: string) {
        io.init(path);
    }

    reader = (highWaterMark: number = 10) => new UserReader({ highWaterMark });
    writer = (highWaterMark: number = 128) => new UserWriter({ highWaterMark });
    reset = io.reset;
}

export default UserStorage;
