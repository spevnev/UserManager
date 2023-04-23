import bind from 'bindings';
const io = bind('io');

export type User = {
    id: number;
    name: string;
    age: number;
};

class UserStorage {
    private users: User[] = [];

    constructor(path: string) {
        io.init(path);
    }

    getAll(): User[] {
        return io.getAll();
    }

    insert(...users: User[]): void {
        this.users.push(...users);
    }

    flush(): void {
        if (this.users.length == 0) return;
        io.append(this.users);
        this.users = [];
    }

    reset(): void {
        io.reset();
    }
}

export default UserStorage;
