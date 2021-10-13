# Synchronization Report

## Operating System | Assignment 2 | 13-10-2021

| Student name | Student number |
| ------------ | -------------- | 
| Jiankai Zheng | s1080484 |
| Shenghang Wang | ? |

## Design
In this chapter the design of our implementation will be discussed.

### Synchronized vector
We have decided to create a special "wrapper" class `synchronized_vector`, where all the read/write operations can be performed on a `vector`. As the name says the operations performed on this vector are guaranteed to be thread safe. This class uses a technique called generics which allows us to reuse this `synchronized_vector` class for different types. This is obviously very convenient as it allows us to use this `synchronized_vector` for both the buffer (containing integer values) and the logger (containing string values). This also allows us to guarantee consistent behavior across all vectors used, and most important of all, thread safety.

### Mutexes
In our solution we use three mutexes: `m_no_readers`, `m_readers` and `m_worker_queue` respectively.

`m_no_readers`: This mutex is used to provide mutual exclusion for the global `readers` variable that keeps count of the number of concurrent readers.

`m_readers`: This mutex is used to determine if there are any readers busy.

`m_worker_queue`: This mutex is used to determine if there are any 'workers' busy or requesting to work. A worker is an entity that wants to perform modification on the data. For instance a worker wanting to perform a write/remove/resize operation.

### Read

### Write

### Remove

### Resize


## Common issues
### Deadlock

### Starvation


## Testing

