# Synchronization Report

## Operating System | Assignment 2 | 14-10-2021

| Student name | Student number |
| ------------ | -------------- | 
| Jiankai Zheng | s1080484 |
| Shenghang Wang | s1034413 |

## Design
In this chapter the design of our implementation will be discussed.

### Synchronized vector
We have decided to create a special "wrapper" class `synchronized_vector`, where all the read/write operations can be performed on a `vector`. As the name says, the operations performed on this vector are guaranteed to be thread safe. This class uses a technique called generics which allows us to reuse this `synchronized_vector` class for different types. This is obviously very convenient as it allows us to use this `synchronized_vector` for both the buffer (containing integer values) and the logger (containing string values). This also allows us to guarantee consistent behavior across all vectors used, and most important of all, thread safety.

### Mutexes
In our solution we use three mutexes: `m_no_readers`, `m_readers` and `m_worker_queue` respectively.

`m_no_readers`: This mutex is used to provide mutual exclusion for the global `readers` variable that keeps count of the number of concurrent readers.

`m_readers`: This mutex is used to determine if there are any readers busy.

`m_worker_queue`: This mutex is used to determine if there are any 'workers' busy or requesting to work. A worker is an entity that wants to perform modification on the data. For instance a worker wanting to perform a write/remove/resize operation.

### Read
The read logic is by far the most complicated in terms of amount of code, and that isn't surprising. The readers has to take into account the amount of concurrent readers and the fact that there are workers and have to provide priority to them.

When a read request is coming in, the first thing it will do is check whether there are any workers busy, or requesting to work. This is done by executing `m_worker_queue.lock()`. If that is the case then it will get stuck there and wait till they are finished, if no worker is busy then it will release the lock again by executing `m_worker_queue.unlock()`. 

Next step is to increment the `readers` count by one, it will first have to acquire the `m_no_readers` mutex. When acquired it will also check if it is the first reader, if so then it will try to acquire the `m_readers` mutex. As explained above, this lock is needed to tell the workers that there are in fact readers busy. 

When all is done, it can finally read its desired value. The user requesting can provide a numeric value at what `index` in the vector they want the value of. 

After finishing with reading it has to decrement the `readers` variable by one, and if it is the last reader it will have to release the `m_readers` mutex to let the workers know that all readers have finished.

### Write
The write logic compared to the read logic is much simpler. At the start of a write request it will first try to acquire the `m_worker_queue` mutex, if acquired then there was no worker currently busy. It can then continue.

Subsequently it will try to acquire the `m_readers` mutex, this is to see if there are any readers currently busy, and if acquired to prevent readers from reading when a writer is busy. If the `m_readers` mutex is acquired it can then finally write to the vector. The value will be written to the end of the vector. 

When finished writing it will release both the `m_readers` and `m_worker_queue` mutex.

### Remove
A removal request is treated the same as write request, in the sense that they are both workers. This basically means that they have to acquire the same mutexes, namely `m_worker_queue` and `m_readers`. When both are acquired it can remove the element at the index that was specified by the user making the request. 

After a successful removal both mutexes are released again.

### Bounded and unbounded buffer
The way bounded and unbounded behaviour is enforced is using two variables, namely, `bounded` and `maxBound`. The `bounded` variable is a boolean and `maxBound` is a numerical value determining the max boundary. These two values are set at the creation of a `synchronized_vector`. 

The `synchronized_vector` provides two constructors, namely one without any arguments (unbounded) and a constructor with one argument (bounded) which is the `bound` value. Using this passed in argument the boundary for the vector is set. This is done using the `maxBound` variable, this variable enforces the max boundary. In this one argument constructor the global `bounded` variable is also set to `true`. This `bounded` variable is used in the operations to determine whether to check if the set boundary has been reached or not.

### Resize
After the creation of the `synchronized_vector` it is also possible to resize the capacity of the `vector`. This resize request just like write and removal is treated as a worker, as it modifies the data/structur of the vector. Therefore it has to go through the same process of acquiring `m_readers` and `m_worker_queue`. 

After having done that it will resize the size using one function. Namely `resize` to set the `size` of the vector. It also sets a `maxBound` variable to use as a max bound. Lastly, it also has to set the `bounded` variable to `true` because it is now bounded by the specified `size` argument.

If the `size` argument equals `-1` then the buffer will be set to be unbounded.

At last it has to release the acquired mutexes again.

## Common issues
### Deadlock
There are no deadlocks in our solution. This is guaranteed by the fact that there is no circulair waiting in our solution. Only a single worker or multiple readers can work at the same time. If a worker is busy then it is guaranteed that its acquired mutexes will be released eventually. If readers are busy then it also guaranteed that its acquired mutex(es) will be released by the last reader. Following from this fact we can conclude that both sides can release their mutexes independent from the other, hence no circular waiting can occur.
### Starvation
In our solution there is no starvation. This is guaranteed by that multiple readers can read at the same time *and* workers have priority over readers. The way it works is that a reader can read if there are any readers busy. Because of the fact that there are readers busy, we know that there can not be any workers. However, if a worker has requested to work, then new readers cannot start reading but has to give workers also a chance. This basically guarantees that both sides get to do their respective work.

## Testing
In this chapter the testing will be discussed. 

### Test cases
We will test whether `synchronized_vector` behaves correctly in a multi-threaded environment. 

If the functionality behaves correctly in a multi-threaded environment, we can safely assume that it performs the same in a single-threaded environment and thus will not always be explicitly tested.

| # | Description | Expected output | Actual output | Rationale |Additional comments |
| --- | --- | --- | --- | --- | --- |
| 1 | Multiple writers request to write [n, n+10] to the buffer | Size of the buffer and logger will be `n*10` (where n is the number of writers). The buffer will contain all the numbers written by the writers and the logger will contain the log messages that a specific number was written to the buffer |The size of the buffer and logger is indeed equal to the number of writes.| With this test we want to prove that writing in a multi-threaded environment works correctly. | see test_1 in the code |
| 2 | Multiple readers try to read from a non zero size buffer | Reading will be performed correctly. If read `n` times, it will receive `n` items. | Multiple readers reading performs as expected. We receive the exact number of items back. | With this test we want to prove that reading in a multi-threaded environment works correctly. | see test_2 in the code|
| 3 | Multiple readers and writers try to work at the same time| Readers and writers can perform their respective operations without any issues. At the end the size of the buffer should equal to `n*i` where `n` is the number of writers and `i` number of items added per writer.| The buffer and logger contains exactly the number of items we expected. | With this test we want to prove that readers and writers can work together in a multi-threaded environment and that there are no deadlocks/starvation. | see test_3 in the code |
| 4 | Removing from the buffer when size equals `0` (empty) | This can not be done and will be logged to the logger. | The appropriate log message is written to the logger | With this test we want to prove that removing from an empty buffer will be handled correctly. | see test_4 in the code |
| 5 | Adding to the buffer when size equals the capacity (full buffer) | This can not be done and will be logged to the logger | The appropriate log message is written to the logger | With this test we want to prove that writing to a full buffer will be handled correctly.| see test_5 in the code |
| 6 | Resizing the buffer while there are readers/writers busy | Resizing is treated as a worker and thus will have to wait for its turn. This is expected to go correctly, and the bound will be set to the specified input. | The actual output mathces the expected output. | With this test we want to prove that resizing the buffer can be done without any issues in a multi-thread environment while there might be readers and workers busy. |see test_6 in the code |
|7| Setting size to -1 makes the buffer unbounded | The buffer will be unbounded and writing an arbitrary number of times will be possible.| The actual output matches the expected out. | With this test we want to prove that it is possible to make the buffer unbounded even after being bounded. |see test_7 in the code|
| 8 | Multiple threads removing at the same time| If removal is requested `n` times then the size of the buffer will decrease by `n` | The actual output matches the expected output. It indeed decreases by `n`. | With this test we want to prove that removing elements in a multi-threaded environment works correctly.|see test_8 in the code. |