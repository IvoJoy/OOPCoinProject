#include <fstream>
#include <iostream>
#include <cstring>
using namespace std;


unsigned computeHash(const unsigned char* memory, int length)
{
    unsigned hash = 0xbeaf;

    for (int c = 0; c < length; c++)
    {
        hash += memory[c];
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

struct User
{
    unsigned id;
    char name[128];
};

struct Transaction
{
    unsigned sender;
    unsigned receiver;
    double coins;
    long long time;
};

struct TransactionBlock
{
    unsigned id;
    unsigned prevBlockId;
    unsigned prevBlockHash;
    int validTransactions;
    Transaction transactions[16];
};


//ako ima file, da ne moje da go promenq s druga programa

size_t getFileSize(ifstream& file)
{
    size_t currentPos = file.tellg();
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(currentPos);
    return fileSize;
}

struct blockChain {

private:
    User* users;
    unsigned usersSize;
    int* userCoins;
    unsigned userCoinsSize = 32;
    unsigned usersCount = 0;
    unsigned currentId;
    TransactionBlock* blocks;
    unsigned blocksSize;
    unsigned blocksCount = 0;

    void resizeUsers()
    {
        User* array2 = new User[usersSize * 2];

        for (size_t i = 0; i < usersSize; i++)
        {
            array2[i] = users[i];
        }

        usersSize = 2 * usersSize;
        delete[] users;
        users = array2;

    }

    void resizeUserCoins()
    {
        int* array2 = new int[userCoinsSize * 2] { -1 };

        for (size_t i = 0; i < userCoinsSize; i++)
        {
            array2[i] = userCoins[i];
        }

        userCoinsSize = 2 * userCoinsSize;
        delete[] userCoins;
        userCoins = array2;

    }

    void resizeBlocks()
    {
        TransactionBlock* array2 = new TransactionBlock[blocksSize * 2];

        for (size_t i = 0; i < blocksSize; i++)
        {
            array2[i] = blocks[i];
        }

        blocksSize = 2 * blocksSize;
        delete[] blocks;
        blocks = array2;

    }

    void readUsers()
    {
        ifstream reader;
        reader.open("users.dat", ios::binary);
        if (!reader.is_open())
        {
            usersSize = 32;
            users = new User[usersSize];
            return;
        }

        usersSize = getFileSize(reader) / sizeof(User);
        users = new User[usersSize];
        reader.read((char*)users, sizeof(User) * usersSize);


        usersCount = usersSize;
    }

    void readBlocks()
    {
        ifstream reader;
        reader.open("blocks.dat", ios::binary);
        if (!reader.is_open())
        {
            blocksSize = 32;
            blocks = new TransactionBlock[blocksSize];
            return;
        }

        blocksSize = getFileSize(reader) / sizeof(TransactionBlock);
        blocks = new TransactionBlock[blocksSize];
        reader.read((char*)blocks, sizeof(TransactionBlock) * blocksSize);

        blocksCount = blocksSize;
    }

    void saveUsers() const
    {
        ofstream writer;
        writer.open("users.dat", ios::binary);

        if (!writer.is_open())
        {
            return;
        }

        writer.write((char*)users, sizeof(User) * usersCount);

    }

    void saveBlocks() const
    {
        ofstream writer;
        writer.open("blocks.dat", ios::binary);

        if (!writer.is_open())
        {
            return;
        }

        writer.write((char*)blocks, sizeof(TransactionBlock) * blocksCount);

    }

    void addBlock() {
        TransactionBlock block;
        block.id = blocks[blocksCount].id + 1;
        block.prevBlockId = blocks[blocksCount].id;
        block.prevBlockHash = computeHash((unsigned char*)&blocks[blocksCount], sizeof(TransactionBlock));
        block.validTransactions = 0;

        if (blocksSize == blocksCount)
        {
            resizeBlocks();
        }
        blocks[blocksCount++] = block;
    }

    void addFirstBlock() {
        TransactionBlock block;
        block.id = 1;
        block.prevBlockId = 1;
        block.prevBlockHash = -1;
        block.validTransactions = 0;

        if (blocksSize == blocksCount)
        {
            resizeBlocks();
        }

        blocks[blocksCount++] = block;
    }

    void addTransactionToBlock(const Transaction& transaction)
    {
        if (blocksCount == 0) {
            addFirstBlock();
        }

        if (blocks[blocksCount - 1].validTransactions == 16)
        {
            addBlock();
        }

        blocks[blocksCount - 1].transactions[blocks[blocksCount - 1].validTransactions++] = transaction;

    }

    void removeUserByIndex(unsigned index)
    {
        for (size_t i = index; i < usersCount - 1; i++)
        {
            users[i] = users[i + 1];
        }
    }

    bool userIsValid(int userID) const
    {
        return userCoins[userID] != -1 || userID == 0;
    }

    int getCurrentId() const {
        int id = 0;

        for (int i = 0; i < blocksCount; i++) {
            for (int j = 0; j < blocks[i].validTransactions; j++) {
                //we use the receiver id because on creation the user receives coins from the admin user =>
                //every user is guaranteed to have at least one transaction where he is the receiver
                int currentId = blocks[i].transactions[j].receiver;
                id = currentId > id ? currentId : id;
            }
        }

        return id + 1;
    }

public:
    //test function to be deleted later
    void printTransactions() {
        for (int i = 0; i < blocksCount; i++) {
            for (int j = 0; j < blocks[i].validTransactions; j++) {
                Transaction curr = blocks[i].transactions[j];
                std::cout << curr.receiver << ' ' << curr.sender << ' ' << curr.coins << std::endl;
            }
        }
    }

    blockChain()
    {
        userCoins = new int[userCoinsSize] {-1};

        readUsers();
        readBlocks();

        currentId = getCurrentId();
    }

    void removeUser()
    {
        char name[128];

        cout << "Enter the name of the user you wish to remove: ";
        cin.getline(name, 128);

        for (size_t i = 0; i < usersCount; i++)
        {
            if (strcmp(name, users[i].name) == 0)
            {
                removeUserByIndex(i);
                cout << "Successfuly removed!";
                usersCount--;
                userCoins[users[i].id] = -1;
                break;
            }
        }

    }

    void createUser()
    {
        User user;
        cout << "Enter name: ";
        cin.getline(user.name, 128);
        user.id = currentId++;

        unsigned cash;
        cout << "Enter how many leva do you want to invest in OOPCoin: ";
        cin >> cash;
        cin.ignore();
        userCoins[user.id] = 420 * cash;

        if (usersCount == usersSize)
        {
            resizeUsers();
        }
        users[usersCount++] = user;

        createTransaction(0, user.id, cash * 420);
    }

    void createTransaction(unsigned senderID, unsigned receiverID, unsigned amountOfCoins)
    {
        if (!(userIsValid(senderID) && userIsValid(receiverID)))
        {
            cout << "Invalid ID";
            return;
        }
        if (amountOfCoins > userCoins[senderID] || amountOfCoins <= 0)
        {
            cout << "Invalid amount";
            return;
        }


        Transaction transaction;
        transaction.sender = senderID;
        transaction.receiver = receiverID;
        transaction.coins = amountOfCoins;

        addTransactionToBlock(transaction);
    }

    ~blockChain()
    {
        saveUsers();
        saveBlocks();
        delete[] users;
        delete[] userCoins;
        delete[] blocks;
    }
};



int main()
{
    blockChain bc;

    bc.printTransactions();
}