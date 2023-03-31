#include <fstream>
#include <iostream>
#include <cstring>
#include <ctime>
using namespace std;
#pragma warning(disable : 4996)


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
    double* userCoins;
    unsigned userCoinsSize = 16;
    unsigned usersCount = 0;
    unsigned currentId;
    TransactionBlock* blocks;
    unsigned blocksSize;
    unsigned blocksCount = 0;


    void removeUser()
    {
        char name[128];
        cout << "Enter the name of the user you wish to remove: ";
        cin.getline(name, 128);

        for (size_t i = 0; i < usersCount; i++)
        {
            if (strcmp(name, users[i].name) == 0)
            {
                createTransaction(users[i].id, 0, userCoins[users[i].id]);
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

        double cash;
        cout << "Enter how many leva do you want to invest in OOPCoin: ";
        cin >> cash;
        cin.ignore();
        userCoins[user.id] = 420 * cash;

        if (usersCount == usersSize)
        {
            resizeUsers();
        }
        users[usersCount++] = user;
        createTransaction(0, user.id, userCoins[user.id]);
    }

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
        double* array2 = new double[userCoinsSize * 2] { -1 };

        for (size_t i = 0; i < userCoinsSize; i++)
        {
            array2[i] = userCoins[i];
        }

        userCoinsSize = 2 * userCoinsSize;
        delete[] userCoins;
        userCoins = array2;
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
        reader.close();
    }

    void getUserCoinsFromArrayOfBlocks()
    {
        inicializeUserCoins();


        for (int i = 0; i < blocksCount; i++)
        {
            for (int j = 0; j < blocks[i].validTransactions; j++)
            {
                Transaction currentTransaction = blocks[i].transactions[j];
                userCoins[currentTransaction.receiver] += currentTransaction.coins;
                userCoins[currentTransaction.sender] -= currentTransaction.coins;
            }
        }
    }

    void inicializeUserCoins()
    {
        for (size_t i = 0; i < usersCount; i++)
        {
            userCoins[users[i].id] = 0;
        }
    }

    void saveUsers() const
    {
        ofstream writer;
        writer.open("users.dat", ios::binary);

        if (!writer.is_open())
        {
            cout << "Couldn't open file!";
            return;
        }

        writer.write((char*)users, sizeof(User) * usersCount);

        writer.close();

    }

    void removeUserByIndex(unsigned index)
    {
        users[index] = users[usersCount - 1];
    }

    bool userIsValid(unsigned userID) const
    {
        return userCoins[userID] != -1 || userID == 0 || userID>currentId;
    }

    unsigned getCurrentId() const {
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

    bool isTransactionValid(unsigned senderID, unsigned receiverID, double amountOfCoins)
    {
        if (!(userIsValid(senderID) && userIsValid(receiverID)))
        {
            return 0;
        }
        if ((amountOfCoins > userCoins[senderID] || amountOfCoins <= 0) && senderID != 0)
        {
            return 0;
        }
    }

    void sendCoinsToOtherUser()
    {
        int senderID;
        int receiverID;
        double amountOfCoins;

        cout << "Tell me the ID of the sender: ";
        cin >> senderID;

        cout << "Tell me the ID of the receiver: ";
        cin >> receiverID;

        cout << "Tell me how many coins are we transfering: ";
        cin >> amountOfCoins;

        cin.ignore();

        if (!(isTransactionValid(senderID, receiverID, amountOfCoins)))
        {

            cout << "Transaction failed! Cause may be that an ID was incorrect or the sender doesn't have enough coins.";
            return;
        }

        createTransaction(senderID, receiverID, amountOfCoins);
        userCoins[senderID] -= amountOfCoins;
        userCoins[receiverID] += amountOfCoins;

    }

    void createTransaction(unsigned senderID, unsigned receiverID, double amountOfCoins)
    {
        Transaction transaction;
        transaction.sender = senderID;
        transaction.receiver = receiverID;
        transaction.coins = amountOfCoins;
        transaction.time = time(nullptr);

        addTransactionToBlock(transaction);
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

    void readBlocks()
    {
        ifstream reader;
        reader.open("blocks.dat", ios::binary);
        if (!reader.is_open())
        {
            blocksSize = 4;
            blocks = new TransactionBlock[blocksSize];
            return;
        }

        blocksSize = getFileSize(reader) / sizeof(TransactionBlock);
        blocks = new TransactionBlock[blocksSize];
        reader.read((char*)blocks, sizeof(TransactionBlock) * blocksSize);

        blocksCount = blocksSize;

        reader.close();
    }
     
    void addBlock()
    {

        TransactionBlock block;
        block.id = blocks[blocksCount - 1].id + 1;
        block.prevBlockId = blocks[blocksCount - 1].id;
        block.prevBlockHash = computeHash((unsigned char*)&blocks[blocksCount - 1], sizeof(TransactionBlock));
        block.validTransactions = 0;

        if (blocksSize == blocksCount)
        {
            resizeBlocks();
        }
        blocks[blocksCount++] = block;
    }

    void saveBlocks()
    {
        ofstream writer;
        writer.open("blocks.dat", ios::binary);

        if (!writer.is_open())
        {
            return;
        }

        writer.write((char*)blocks, sizeof(TransactionBlock) * blocksCount);

        writer.close();

    }

    void addFirstBlock() {
        TransactionBlock block;
        block.id = 1;
        block.prevBlockId = 1;
        block.prevBlockHash = 0;
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

    void getWealthiestUsers(unsigned n)
    {
        User* wealthiestUsers = new User[n];

        unsigned index = 0;

        for (size_t i = 0; i < usersCount; i++)
        {
            for (size_t j = 0; j < n; j++)
            {
                if (j == index)
                {
                    wealthiestUsers[index++] = users[i];
                    break;
                }

                if (userCoins[users[i].id] >= userCoins[wealthiestUsers[j].id])
                {
                    unsigned startIndex = index == n ? index - 1 : index;
                    shiftRight(wealthiestUsers, startIndex, j);
                    index = index < n ? index + 1 : index;
                    wealthiestUsers[j] = users[i];
                    break;
                }

            }
        }

        unsigned length = index == n ? index - 1 : index;
        sendArrayToTextFile(wealthiestUsers, length);

        delete[] wealthiestUsers;
    }

    void sendArrayToTextFile(User* usersArray, unsigned arrayLength)
    {
        ofstream writer;
        char filename[256]{ '\0' };
        long long timeNow = time(nullptr);
        char timeToChar[128] = { '\0' };
        sprintf(timeToChar, "%d", timeNow);
        strcpy(filename, "wealthiest-users_");
        strcat(filename, timeToChar);
        strcat(filename, ".txt");
        writer.open(filename, ios::app);

        if (!writer.is_open())
        {
            cout << "Couldn't open file!" << endl;
            return;
        }

        for (size_t i = 0; i <= arrayLength; i++)
        {
            writer << "username: " << usersArray[i].name << ", " << "amount of coins: " << userCoins[usersArray[i].id] << endl;
        }

        cout << "Successfuly created a file!";
        writer.close();
    }

    void shiftRight(User* users, unsigned startIndex, unsigned endIndex)
    {
        for (size_t i = startIndex; i > endIndex; i--)
        {
            users[i] = users[i - 1];
        }
    }

    //this function sacrifices code readability to save some precious time, because this algorithm minimizes the number of swaps needed.
    void insertBlockID(unsigned* biggestBlockIDs, unsigned size, double coins, unsigned& numberOfIDs, double* sumsOfBlocks, unsigned currentID)
    {
        for (size_t i = 0; i < size; i++)
        {
            if (coins > sumsOfBlocks[i])
            {
                unsigned startIndex = numberOfIDs == size ? numberOfIDs - 1 : numberOfIDs;
                shiftRight(sumsOfBlocks, startIndex, i);
                sumsOfBlocks[i] = coins;
                biggestBlockIDs[i] = currentID;
                numberOfIDs = numberOfIDs < size ? numberOfIDs + 1 : numberOfIDs;
                break;
            }
        }
    }

    void getBiggestBlocks(unsigned size)
    {
        unsigned* biggestBlocksIDs = new unsigned[size];
        double* sumsOfBlocks = new double[size] {0};

        unsigned numberOfIDs = 0;

        double sumOfCoinsFromTransactions = 0;

        for (int i = 0; i < blocksCount; i++)
        {
            for (int j = 0; j < blocks[i].validTransactions; j++)
            {
                sumOfCoinsFromTransactions += blocks[i].transactions[j].coins;
            }

            insertBlockID(biggestBlocksIDs, size, sumOfCoinsFromTransactions, numberOfIDs, sumsOfBlocks, i + 1);

            sumOfCoinsFromTransactions = 0;
        }

        sendArraysToTextFile(biggestBlocksIDs, sumsOfBlocks, numberOfIDs);

        delete[] biggestBlocksIDs;
        delete[] sumsOfBlocks;

    }
    
    void shiftRight(double* sumsOfBlocks, unsigned startIndex, unsigned endIndex)
    {
        for (size_t i = startIndex; i > endIndex; i--)
        {
            sumsOfBlocks[i] = sumsOfBlocks[i - 1];
        }
    }

    void sendArraysToTextFile(unsigned* biggestBlocksIDs, double* sumsOfBlocks, unsigned arrayLength)
    {
        ofstream writer;
        char filename[256]{ '\0' };
        long long timeNow = time(nullptr);
        char timeToChar[128] = { '\0' };
        sprintf(timeToChar, "%d", timeNow);
        strcpy(filename, "biggest-blocks_");
        strcat(filename, timeToChar);
        strcat(filename, ".txt");
        writer.open(filename, ios::app);

        if (!writer.is_open())
        {
            cout << "Couldn't open file!" << endl;
            return;
        }

        for (size_t i = 0; i < arrayLength; i++)
        {
            writer << "Sum of block with ID " << biggestBlocksIDs[i] << " is " << sumsOfBlocks[i] << " coins." << endl;
        }

        cout << "Successfuly created a file!";
        writer.close();
    }

    void verifyTransactions()
    {
        bool flag = true;
        for (size_t i = 1; i < blocksCount; i++)
        {
            if (blocks[i].prevBlockHash != computeHash((unsigned char*)&blocks[i - 1], sizeof(TransactionBlock)))
            {
                flag = false;
                cout << "The block which is placed on position " << i << " in the blockchain is corrupted!" << endl;
            }
        }
        if (flag)
        {
            cout << "Everything is fine with your blocks!";
        }
    }

   /* void printBlocks()
    {
        for (size_t i = 0; i < blocksCount; i++)
        {
            cout << blocks[i].prevBlockHash << ' ';
        }
    }*/


public:
    
    void printTransactions() {
        for (int i = 0; i < blocksCount; i++) {
            for (int j = 0; j < blocks[i].validTransactions; j++) {
                Transaction curr = blocks[i].transactions[j];
                cout << curr.receiver << ' ' << curr.sender << ' ' << curr.coins << ' ' << curr.time << endl;
            }
            cout << "kray na bloka" << endl;
        }
    }
    void printUsers() {
        for (int i = 0; i < usersCount; i++) {
            
           User currentUser = users[i];
          cout << currentUser.id << ' ' << currentUser.name << ' ' << userCoins[currentUser.id]<< endl;
            
        }
    }

    blockChain()
    {
        userCoins = new double[userCoinsSize] {-1};

        readUsers();
        readBlocks();
        getUserCoinsFromArrayOfBlocks();
        currentId = getCurrentId();
        

    }

    //void printArray(User* users, unsigned size) // have to ask
    //{
    //    for (size_t i = 0; i < size; i++)
    //    {
    //        cout << users[i].name << ' ' << userCoins[users[i].id] << endl;
    //    }
    //}  // to be deleted

    //void printArray(double* sumsOfBlocks, unsigned* biggestBlocksIDs, unsigned size) 
    //{
    //    for (size_t i = 0; i < size; i++)
    //    {
    //        cout << "Sum of block with ID " << biggestBlocksIDs[i] << " is " << sumsOfBlocks[i] << endl;
    //    }
    //}

    void interface()
    {
        cout << "Hello and welcome to OOPCoin. Valid commands are: create-user, remove-user, send-coins,\nwealthiest-users, biggest-blocks, verify-transactions and exit." << endl;
        
        
        while (1)
        {
            cout << "\nPlease give me a command to execute:" << endl;
            char command[21];
            cin.getline(command, 20);
            if (cin.fail())
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Not a valid command! Should be less than 20 characters." << endl;
                continue;
            }

            if (!strcmp(command, "create-user"))
            {
                createUser();
            }

            else if (!strcmp(command, "remove-user"))
            {
                removeUser();
            }

            else if (!strcmp(command, "send-coins"))
            {
                sendCoinsToOtherUser();
            }

            else if (!strcmp(command, "wealthiest-users"))
            {
                unsigned size;
                cout << "Tell me how many of the wealthiest users you wish to see ranked:" << endl; // if you write a number, bigger than your count of users, it will create the file with the amount of users you have only.
                cin >> size;
                cin.ignore();
                getWealthiestUsers(size);
            }

            else if (!strcmp(command, "biggest-blocks"))
            {
                unsigned size;
                cout << "Tell me how many of the biggest blocks you wish to see ranked:" << endl; // if you write a number, bigger than your count of blocks, it will create the file with the amount of blocks you have only.
                cin >> size;
                cin.ignore();
                getBiggestBlocks(size);
            }

            else if (!strcmp(command, "verify-transactions"))
            {
                verifyTransactions();
            }

            else if (!strcmp(command, "exit"))
            {
                return;
            }

            else
            {
                cout << "Invalid command! Valid commands are: create-user, remove-user, send-coins, wealthiest-users, biggest-blocks, verify-transactions and exit";
                
            }
        }
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
    blockChain bbc;
    bbc.interface();
}
