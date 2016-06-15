# Instance Tracking Library (c++)
Simple and fast tracking and iteration of c++ object instances.

## Features
  * Simple integration with source code (2 lines required in user class to enable all functionality).
  * Fast adding and removal of instances to the list (no memory allocation).
  * Fast iteration (List nodes exist with object instances to reduce cache misses).
  * Supports standard iterator syntax.
  * Threadsafe (optional, see `INST_TRACKING_THREAD_TEST` define in source).
  * Supports sorting of instance list using custom sort functions (undocumented, see `List::Sort()` in source).

## Basic Usage
1. Add the header file to your project
2. Add an `RTM::Instance::Tracker` to the class you wish to track and initialize it in the constructor

    ```c++
    using namespace RTM::Instance;
    ...
    
    class TrackableClass{
    public:
    TrackableClass(int value = 0)
        : InstanceTracker(this)                         // init tracker with 'this' pointer
        , IntegerValue(value)
    {}
    
    private:
        Tracker<TrackableClass> InstanceTracker;        // Tracker member variable
        int IntegerValue;
    };
    ```
3. Instances will be added to an `RTM::Instance::List` on construction and removed on destruction:
  
    ```c++
    using namespace RTM::Instance;
    ...
    
    int numInstances = List<TrackableClass>::GetNum();
    ```

4. The list can be easily iterated:
  
    ```c++
    using namespace RTM::Instance;
    ...
    
    for (auto&& itr : List<TestClass>())
        cout << itr->IntegerValue << endl;
        
    // alternatively:
    for (auto itr = List<TestClass>::begin(); itr != List<TestClass>::end(); ++itr)
        cout << itr->IntegerValue << endl;
    ```
  
## Known Issues
  * Use of static `begin()` and `end()` functions in `RTM::Instance::List` causes a warning in Visual Studio with warning level of /W4 or higher. See https://connect.microsoft.com/VisualStudio/feedback/details/1322808/warning-w4-with-compiler-generated-local-variable
  * List manipulation is unnecessarily implemented in templated functions, causing code bloat if you need to track multiple classes.
  * Facility for deleting instances during iteration has slightly clumsy syntax.
