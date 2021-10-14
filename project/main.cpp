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
    //start critical section
    buffer.push_back(item);
    //end critical section
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
    //start critical section
    buffer.erase(buffer.begin() + index);
    //end critical section
    m_readers.unlock();
    m_worker_queue.unlock();
  }

  T read(int index)
  {
    const int size = this->size();

    if (index < 0 || index >= size)
    {
      throw invalid_argument("Error: provided index is out of bounds! Provided index: " + to_string(index) + ". Actual size: " + to_string(size));
    }

    m_worker_queue.lock(); //check whether there are modifications are taking place or is being requested, if not just release the lock again
    m_worker_queue.unlock();

    m_no_readers.lock();
    //start critical section
    if (readers == 0)
    {
      m_readers.lock(); //if it is the first reader then signal that there are readers busy
    }
    readers++;
    //end critical section
    m_no_readers.unlock();

    //start critical section
    T value = buffer.at(index);
    //end critical section

    m_no_readers.lock();
    //start critical section
    readers--;

    if (readers == 0)
    {
      //end critical section
      m_readers.unlock(); //if it was the last reader then signal that there are no readers busy anymore
    }

    m_no_readers.unlock();

    return value;
  }

  void resize(int size)
  {
    if (size < 0)
    {
      throw new invalid_argument("Error: provided size was not a positive number. Provided size: " + to_string(size));
    }
    m_worker_queue.lock(); //signal that there is a request for resize
    m_readers.lock();      //wait till all readers are done reading
    //start critical section
    bounded = true;
    buffer.reserve(size);
    //end critical section
    m_readers.unlock();
    m_worker_queue.unlock();
  }

  int size()
  {
    //treat reading the size as a reading request
    m_worker_queue.lock();
    m_worker_queue.unlock();

    m_no_readers.lock();
    //start critical section
    if (readers == 0)
    {
      m_readers.lock(); //if it is the first reader then signal that there are readers busy
    }
    readers++;
    //end critical section
    m_no_readers.unlock();

    //start critical section
    int size = buffer.size();
    //end critical section
    m_no_readers.lock();
    //start critical section
    readers--;

    if (readers == 0)
    {
      //end critical section
      m_readers.unlock(); //if it was the last reader then signal that there are no readers busy anymore
    }

    m_no_readers.unlock();

    return size;
  }

private:
  bool capacity_reached()
  {
    //treat reading the capacity as a reading request
    m_worker_queue.lock();
    m_worker_queue.unlock();

    m_no_readers.lock();
    //start critical section
    if (readers == 0)
    {
      m_readers.lock(); //if it is the first reader then signal that there are readers busy
    }
    readers++;
    //end critical section
    m_no_readers.unlock();
    //start critical section

    bool capacity_reached = bounded && (buffer.size() == buffer.capacity());
    cout << "bounded: " << bounded << " buffer size: " << buffer.size() << " capacity: " << buffer.capacity() << "\n";
    //end critical section

    m_no_readers.lock();
    //start critical section
    readers--;

    if (readers == 0)
    {
      //end critical section

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

void readFromBuffer(int index)
{
  try
  {
    buffer.read(index);
    logger.write("Success: Reading at index " + to_string(index) + " was successful.");
  }
  catch (const invalid_argument ia)
  {
    logger.write(ia.what());
  }
}

void removeFromBuffer(int index)
{
  try
  {
    buffer.remove(index);
    logger.write("Success: Removing at index " + to_string(index) + " was successful.");
  }
  catch (const invalid_argument ia)
  {
    logger.write(ia.what());
  }
}

void writeFromToBuffer(int start, int end)
{
  for (int i = start; i < end; i++)
  {
    writeToBuffer(i);
  }
}

void readFromToBuffer(int start, int end)
{
  for (int i = start; i < end; i++)
  {
    readFromBuffer(i);
  }
}

void resizeBuffer(int size)
{
  try
  {
    buffer.resize(size);
    logger.write("Success: Resizing to size " + to_string(size) + " was successful.");
  }
  catch (const invalid_argument ia)
  {
    logger.write(ia.what());
  }
}

/**
 * TESTS START HERE 
 */

//for test readability.
void assertTrue(bool assertion)
{
  if (!assertion)
  {
    cout << "Assertion was false! \n";
  }
  else
  {
    cout << "Assertion was true! \n";
  }
}

void logger_contains(string text)
{
  bool contains = false;

  for (int i = 0; i < logger.size(); i++)
  {
    if (logger.read(i) == text)
    {
      contains = true;
    }
  }

  assertTrue(contains);
}

void test_4()
{
  assertTrue(logger.size() == 0);
  assertTrue(buffer.size() == 0);

  removeFromBuffer(0);

  logger_contains("Error: provided index is out of bounds! Provided index: 0. Actual size: 0");
}

void test_5()
{
  resizeBuffer(1);
  assertTrue(logger.size() == 0);
  assertTrue(buffer.size() == 0);

  //should be okay
  writeToBuffer(1);
  //third assertion should be false
  logger_contains("Error: Writing to the buffer was unsuccessful. Cause: the buffer is full!");

  writeToBuffer(2);
  //true
  logger_contains("Error: Writing to the buffer was unsuccessful. Cause: the buffer is full!");
}

void test_6()
{
  //populate buffer so that reading won't go wrong.
  writeFromToBuffer(0, 10);
  assertTrue(logger.size() == 10);
  assertTrue(buffer.size() == 10);

  thread t1 = thread(writeFromToBuffer, 0, 10);
  thread t2 = thread(writeFromToBuffer, 10, 20);
  thread t3 = thread(resizeBuffer, 5);
  thread t4 = thread(readFromBuffer, 5);
  thread t5 = thread(readFromToBuffer, 5, 10);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();

  assertTrue(buffer.size() == 5);
}
/**
 * TESTS END HERE 
 */

#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
  // cout << "size: " << buffer.size();
  // thread t3 = thread(resizeBuffer);
  // thread t1 = thread(writeToBuffer, 1);
  // thread t2 = thread(writeToBuffer, 1);
  // t1.join();
  // t2.join();
  // t3.join();

  // ofstream myfile;
  // myfile.open("example.txt", std::ios_base::app);
  // // cout << logger.size();
  // cout << buffer.size();
  // for (int i = 0; i < logger.size(); i++)
  //   myfile << logger.read(i) << "\n";

  // myfile.close();
  // test_4();
  // test_5();
  test_6();
  return 0;
}
