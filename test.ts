import UserStorage, { User } from './src';

const main = () => {
    const storage = new UserStorage('./test.storage');

    let arr: User[] = [];
    storage.reset();

    for (let i = 0; i < 100000; i++) {
        if (Math.random() < 0.05) {
            const arr2 = storage.getAll();
            for (let i = 0; i < arr2.length; i++) if (JSON.stringify(arr[i]) !== JSON.stringify(arr2[i])) return console.error('Error', arr.length, arr[i], arr2[i]);
            storage.reset();
            arr = [];
        } else {
            const users = Math.random() * 20;
            for (let i = 0; i < users; i++) {
                const user = { id: Math.round(Math.random() * 4_294_967_296), age: Math.round(Math.random() * 100), name: 'Test' + Math.random() };
                storage.insert(user);
                arr.push(user);
            }
            storage.flush();
        }
    }
};

main();
