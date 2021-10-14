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

  mutex m_no_readers;   //mutex for mutual exclusion of readers variable
  mutex m_readers;      //mutex for determining if readers can continue to write
  mutex m_worker_queue; //mutex for determining if there is a request for modification like writing or removing
  bool bounded;

public:
  synchronized_vector(int bound)
  {
    bounded = true;
    buffer.reserve(bound);
  }

  synchronized_vector() //unbounded buffer
  {
    bounded = false;
  }
  void write(T item)
  {
    if (this->capacity_reached())
    {
      throw runtime_error("Error: Writing to the buffer was unsuccessful. Cause: the buffer is full!");
    }

    m_worker_queue.lock(); // signal that a writer wants to write
    m_readers.lock();      //wait till all readers are done reading
    buffer.push_back(item);
    m_readers.unlock(); //signal that the writer is done writing so that readers can start reading or another writer is allowed to write
    m_worker_queue.unlock();
  }

  void remove(int index)
  {
    const int size = this->size();
    if (index < 0 || index >= size)
    {
      throw invalid_argument("Error: provided index is out of bounds! Provided index: " + to_string(index) + ". Actual size: " + to_string(size));
    }

    m_worker_queue.lock(); //signal that there is a request for removal
    m_readers.lock();      //wait till all readers are done reading
    buffer.erase(buffer.begin() + index);
    m_readers.unlock();
    m_worker_queue.unlock();
  }

  T read(int index)
  {
    const int size = this->size();;
    if (index < 0 || index >= size)
    {
      throw invalid_argument("Error: provided index is out of bounds! Provided index: " + to_string(index) + ". Actual size: " + to_string(size));
    }

    m_worker_queue.lock(); //check whether there are modifications are taking place or is being requested, if not just release the lock again
    m_worker_queue.unlock();

    m_no_readers.lock();
    if (readers == 0)
    {
      m_readers.lock(); //if it is the first reader then signal that there are readers busy
    }
    readers++;
    m_no_readers.unlock();

    T value = buffer.at(index);

    m_no_readers.lock();
    readers--;

    if (readers == 0)
    {
      m_readers.unlock(); //if it was the last reader then signal that there are no readers busy anymore
    }

    m_no_readers.unlock();

    return value;
  }

  void resize(int size)
  {
    m_worker_queue.lock(); //signal that there is a request for resize
    m_readers.lock();      //wait till all readers are done reading
    bounded = true;
    buffer.resize(size);
    buffer.reserve(size);
    m_readers.unlock();
    m_worker_queue.unlock();
  }

  int size()
  {
    //treat reading the size as a reading request
    m_worker_queue.lock();
    m_worker_queue.unlock();

    m_no_readers.lock();
    if (readers == 0)
    {
      m_readers.lock(); //if it is the first reader then signal that there are readers busy
    }
    readers++;
    m_no_readers.unlock();

    int size = buffer.size();

    m_no_readers.lock();
    readers--;

    if (readers == 0)
    {
      m_readers.unlock(); //if it was the last reader then signal that there are no readers busy anymore
    }

    m_no_readers.unlock();

    return size;
  }

  private:
  bool capacity_reached(){
        //treat reading the capacity as a reading request
    m_worker_queue.lock();
    m_worker_queue.unlock();

    m_no_readers.lock();
    if (readers == 0)
    {
      m_readers.lock(); //if it is the first reader then signal that there are readers busy
    }
    readers++;
    m_no_readers.unlock();

    bool capacity_reached = bounded && buffer.size() == buffer.capacity();

    m_no_readers.lock();
    readers--;

    if (readers == 0)
    {
      m_readers.unlock(); //if it was the last reader then signal that there are no readers busy anymore
    }

    m_no_readers.unlock();

    return capacity_reached;
  }
};

synchronized_vector<string> logger;
synchronized_vector<int> buffer;
void writeToBuffer(int num)
{
    try
    {
      buffer.write(num);
      logger.write("Success: Writing " + to_string(num) + " was successful.");
    }
    catch (const runtime_error re)
    {
      logger.write(re.what());
    }
}
#include <iostream>
#include <fstream>

void resizeBuffer()
{
    buffer.resize(5);
}
int main(int argc, char *argv[])
{
  cout << "size: " << buffer.size();
  thread t3 = thread(resizeBuffer);
  thread t1 = thread(writeToBuffer, 1);
  thread t2 = thread(writeToBuffer, 1);
  t1.join();
  t2.join();
  t3.join();

  ofstream myfile;
  myfile.open("example.txt", std::ios_base::app);
  // cout << logger.size();
  cout << buffer.size();
  for (int i = 0; i < logger.size(); i++)
    myfile << logger.read(i) << "\n";

  myfile.close();
  return 0;
}
