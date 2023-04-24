import UserStorage, { User } from './src';

const main = async () => {
    const storage = new UserStorage('./test.storage');

    let arr: User[] = [];
    storage.reset();

    for (let i = 0; i < 1000000; i++) {
        if (Math.random() < 0.01) {
            const reader = storage.reader();
            const users: User[] = [];
            await new Promise(res => {
                reader.on('data', (data: User[]) => users.push(...data));
                reader.on('end', () => res(null));
            });
            for (let i = 0; i < Math.max(arr.length, users.length); i++) if (JSON.stringify(arr[i]) !== JSON.stringify(users[i])) return console.error('Error', arr[i], users[i]);
            storage.reset();
            arr = [];
        } else {
            const users = Math.floor(Math.random() * 20);
            const writer = storage.writer();
            for (let i = 0; i < users; i++) {
                const user = { id: Math.round(Math.random() * 4_294_967_296), age: Math.round(Math.random() * 100), name: 'Test' + Math.random() };
                writer.write(user);
                arr.push(user);
            }
            writer.end();
        }
    }
};

main();
