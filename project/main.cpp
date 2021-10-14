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
  int readers = 0; //number of readers

  mutex m_no_readers;   //mutex for mutual exclusion of readers variable
  mutex m_readers;      //mutex for determining if readers can continue to write
  mutex m_worker_queue; //mutex for determining if there is a request for modification like writing or removing
  bool bounded;         //determines whether the buffer is bounded or not
  int maxBound = 0;     //determines the max boundary of the buffer, initially zero

public:
  synchronized_vector(int bound)
  {
    bounded = true;
    maxBound = bound;
  }

  synchronized_vector() //unbounded buffer
  {
    bounded = false;
  }
  void write(T item)
  {

    m_worker_queue.lock(); // signal that a writer wants to write
    m_readers.lock();      //wait till all readers are done reading
    //start critical section
    bool capacity_reached = bounded && (buffer.size() == maxBound);

    if (capacity_reached)
    {
      m_readers.unlock();
      m_worker_queue.unlock();
      throw runtime_error("Error: Writing to the buffer was unsuccessful. Cause: the buffer is full!");
    }
    buffer.push_back(item);
    //end critical section
    m_readers.unlock(); //signal that the writer is done writing so that readers can start reading or another writer is allowed to write
    m_worker_queue.unlock();
  }

  void remove(int index)
  {
    m_worker_queue.lock(); //signal that there is a request for removal
    m_readers.lock();      //wait till all readers are done reading
    //start critical section
    int size = buffer.size();
    if (index < 0 || index >= size)
    {
      m_readers.unlock();
      m_worker_queue.unlock();
      throw invalid_argument("Error: provided index is out of bounds! Provided index: " + to_string(index) + ". Actual size: " + to_string(size));
    }
    buffer.erase(buffer.begin() + index);
    //end critical section
    m_readers.unlock();
    m_worker_queue.unlock();
  }

  T read(int index)
  {

    m_worker_queue.lock(); //check whether there are modifications are taking place or is being requested, if not just release the lock again
    m_worker_queue.unlock();

    m_no_readers.lock();
    //start critical section
    if (readers == 0)
    {
      m_readers.lock(); //if it is the first reader then signal that there are readers busy
    }

    int size = buffer.size();
    if (index < 0 || index >= size)
    {
      if (readers == 0)
      {
        m_readers.unlock();
      }
      m_no_readers.unlock();
      throw invalid_argument("Error: provided index is out of bounds! Provided index: " + to_string(index) + ". Actual size: " + to_string(size));
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
    if (size < -1)
    {
      throw new invalid_argument("Error: provided size was not a valid number. Provided size: " + to_string(size));
    }
    m_worker_queue.lock(); //signal that there is a request for resize
    m_readers.lock();      //wait till all readers are done reading
    //start critical section
    if (size == -1)
    {
      bounded = false;
    }
    else if (size < buffer.size())
    {
      buffer.resize(size);
    }

    bounded = true;
    maxBound = size;
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
  if (assertion)
  {
    cout << "Assertion was true! \n";
  }
  else
  {
    cout << "Assertion was false! \n";
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

void test_1()
{
  assertTrue(buffer.size() == 0);
  assertTrue(logger.size() == 0);

  thread t1 = thread(writeFromToBuffer, 0, 10);
  thread t2 = thread(writeFromToBuffer, 10, 20);
  thread t3 = thread(writeFromToBuffer, 20, 30);
  thread t4 = thread(writeFromToBuffer, 30, 40);
  thread t5 = thread(writeFromToBuffer, 40, 50);

  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();

  assertTrue(buffer.size() == 50);
  assertTrue(logger.size() == 50);
}

vector<int> results;

void readBufferAndPushToResults()
{
  //we are aware that calling buffer.size() from here might cause issues if the size changed in the meantime
  //however since this is only used for test_2 and we are only reading, this is not an issue
  for (int i = 0; i < buffer.size(); i++)
  {
    results.push_back(buffer.read(i));
  }
}

void test_2()
{
  assertTrue(buffer.size() == 0);
  assertTrue(logger.size() == 0);

  writeFromToBuffer(0, 10);

  thread t1(readBufferAndPushToResults);
  thread t2(readBufferAndPushToResults);
  thread t3(readBufferAndPushToResults);
  thread t4(readBufferAndPushToResults);
  thread t5(readBufferAndPushToResults);

  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();

  assertTrue(results.size() == 50);
}

void test_3()
{

  assertTrue(buffer.size() == 0);
  assertTrue(logger.size() == 0);

  writeFromToBuffer(0, 50);

  thread t1 = thread(writeFromToBuffer, 0, 10);
  thread t2 = thread(writeFromToBuffer, 10, 20);
  thread t3 = thread(writeFromToBuffer, 20, 30);
  thread t4 = thread(writeFromToBuffer, 30, 40);
  thread t5 = thread(writeFromToBuffer, 40, 50);
  thread t6 = thread(readFromToBuffer, 0, 10);
  thread t7 = thread(readFromToBuffer, 10, 20);
  thread t8 = thread(readFromToBuffer, 20, 30);
  thread t9 = thread(readFromToBuffer, 30, 40);
  thread t10 = thread(readFromToBuffer, 40, 50);

  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  t6.join();
  t7.join();
  t8.join();
  t9.join();
  t10.join();

  assertTrue(buffer.size() == 100);
  assertTrue(logger.size() == 150); //50 from initial write + 100 from the threads
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
  assertTrue(logger.size() == 0);
  assertTrue(buffer.size() == 0);
  resizeBuffer(1);

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

void test_7()
{
  resizeBuffer(1);                //first make it bounded
  assertTrue(buffer.size() == 0); //should still be empty
  resizeBuffer(-1);               // now make it unbounded
  writeFromToBuffer(0, 10);       //should be possible because it's unbounded

  assertTrue(buffer.size() == 10);
}
/**
 * TESTS END HERE 
 */

int main(int argc, char *argv[])
{
  //only run 1 test at a time.
  //running multiple will produce incorrect test results as the buffer and logger is not cleared after each test
  test_1();
  // test_2();
  // test_3();
  // test_4();
  // test_5();
  // test_6();
  // test_7();
  return 0;
}
