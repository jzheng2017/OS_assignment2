/**
  * Assignment: synchronization
  * Operating Systems
  */

/**
  Hint: F2 (or Control-klik) on a functionname to jump to the definition
  Hint: Ctrl-space to auto complete a functionname/variable.
  */

// function/class definitions you are going to use
#include <algorithm>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// although it is good habit, you don't have to type 'std::' before many objects by including this line
using namespace std;

template <typename T>
class synchronized_vector
{
  vector<T> buffer;
  int readers = 0; //numer of readers
 
  mutex m_no_readers; //mutex for mutual exclusion of readers variable
  mutex m_readers; //mutex for determining if readers can continue to write
  mutex m_queue; //mutex for determining if a writer is writing or wants to write

public:
  void write(T item)
  {
    m_queue.lock(); // signal that a writer wants to write
    m_readers.lock();  //wait till all readers are done reading
    buffer.push_back(item);
    m_readers.unlock(); //signal that the writer is done writing so that readers can start reading or another writer is allowed to write
    m_queue.unlock(); 
  }

  T read(int index)
  {
    m_queue.lock(); //check whether there are writers busy or if a writer wants to start writing, if not just release the lock again
    m_queue.unlock();

    m_no_readers.lock();
    if (readers == 0)
    {
      m_readers.lock(); //if it is the first reader then signal that there are readers busy
    }
    readers++;
    m_no_readers.unlock();

    T value = buffer[index];

    m_no_readers.lock();
    readers--;

    if (readers == 0)
    {
      m_readers.unlock(); //if it was the last reader then signal that there are no readers busy anymore
    }

    m_no_readers.unlock();
   
    return value;
  }

  int size()
  {
    return buffer.size();
  }
};

synchronized_vector<string> logger = {};
synchronized_vector<int> buffer;

void writeToLogger(string msg)
{
  for (int i = 0; i < 10; i++)
  {
    logger.write(msg);
  }
}
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
  thread t1 = thread(writeToLogger, "first thread");
  thread t2 = thread(writeToLogger, "second thread");

  t1.join();
  t2.join();

  // cout << logger.size();
  ofstream myfile;
  myfile.open("example.txt", std::ios_base::app);
  // myfile << "Writing this to a file.\n";

  for (int i = 0; i < 20; i++)
    myfile << logger.read(i) << "\n";

  myfile.close();
  return 0;
}
