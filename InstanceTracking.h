// Copyright (C) Richard Meredith 2016
// Licensed under the MIT License https://opensource.org/licenses/MIT
//
// $Id: //depot/Utilities/C++/ObjectInstanceTracker/ObjectInstanceTracker/Source/InstanceTracking.h#9 $
// $DateTime: 2016/06/15 22:30:33 $ 
// $Change: 104 $

#pragma once

#define INST_TRACKING_USING_STD_MUTEX

#if defined(INST_TRACKING_USING_STD_MUTEX)
#include <mutex>
#endif

namespace RTM
{
namespace Instance
{

template<typename T> struct Tracker;	// Added as member variable to class T to track all instances of T
template<typename T> struct List;		// Presents a container-like interface to the application
template<typename T> struct Iterator;	// Iterates through the List of instances


////////////////////////////////////////////////////////////////////
// Tracker - used to track all objects of a given class or struct
	
template<typename T> 
struct Tracker
{
	Tracker(T* instance = nullptr);
	~Tracker() {EndTracking();}

	bool BeginTracking(T* instance) {return !TrackedInstance && List<T>::Add(this, instance);}
	bool EndTracking() {return TrackedInstance && List<T>::Remove(this);}
		
	bool IsTracking() const {return TrackedInstance != nullptr;}
	T* GetTrackedInstance() const {return TrackedInstance;}

private:
	T* TrackedInstance;
	Tracker<T>* Prev;
	Tracker<T>* Next;

	friend Iterator<T>;
	friend List<T>;
};

template<typename T>
Tracker<T>::Tracker(T* instance)
	: TrackedInstance(nullptr)
	, Prev(nullptr)
	, Next(nullptr)
{
	BeginTracking(instance);
}


////////////////////////////////////////////////////////////////////////////
// List - Presents a container-like interface to the application

template<typename T>
struct List
{
	typedef bool(*ComparisonFunc)(const T& a, const T& b);

	static unsigned int GetNum() {return Num;}

	static Iterator<T> begin() {Lock(); return Iterator<T>(First, true);}
	static Iterator<T>& end() {return Iterator<T>::EmptyIterator;}

	static void Sort(ComparisonFunc comparisonFunction = Less);

private:
	static Tracker<T>* First;
	static Tracker<T>* Last;
	static unsigned int Num;

#if defined(INST_TRACKING_USING_STD_MUTEX)
	static std::mutex Mutex;
#endif
	static bool Add(Tracker<T>* node, T* inst);
	static bool Remove(Tracker<T>* node, bool takeLock = true);

	static bool Lock();
	static bool Unlock();

	static bool Less(const T& a, const T& b) {return a < b;}
	static void SwapWithNext(Tracker<T>* node);

	friend Tracker<T>;
	friend Iterator<T>;
};

template<typename T> unsigned int List<T>::Num = 0;
template<typename T> Tracker<T>* List<T>::First = nullptr;
template<typename T> Tracker<T>* List<T>::Last = nullptr;

#if defined(INST_TRACKING_USING_STD_MUTEX)
template<typename T> 
std::mutex List<T>::Mutex;

template<typename T> 
inline bool List<T>::Lock()
{
	Mutex.lock(); 
	return true; 
}

template<typename T> 
inline bool List<T>::Unlock()	
{
	Mutex.unlock();
	return true;
}
#else	
// default non-threadsafe implementation
template<typename T> 
inline bool List<T>::Lock() {return true;}

template<typename T> 
inline bool List<T>::Unlock() {return true;}
#endif

template<typename T>
bool List<T>::Add(Tracker<T>* node, T* inst)
{
	if(node && inst && Lock())
	{
		Num++;
		node->TrackedInstance = inst;
		node->Prev = Last;
		node->Next = nullptr;
		Last = node;
		
		(node->Prev ? node->Prev->Next : First) = node;

		Unlock();
		return true;
	}
	return false;
}

template<typename T>
bool List<T>::Remove(Tracker<T>* node, bool takeLock)
{
	if(node && (!takeLock || Lock()))
	{
		(node->Prev ? node->Prev->Next : First) = node->Next;
		(node->Next ? node->Next->Prev : Last) = node->Prev;

		*node = Tracker<T>();
		Num--;

		return !takeLock || Unlock();
	}
	return false;
}

template<typename T>
void List<T>::Sort(ComparisonFunc comparisonFunction)
{
	if(Num < 2)
		return;

	// create the iterator outside of the loop - locking is scoped.
	Iterator<T> itr = begin();

	for(unsigned int i=0, maxItrs=Num-1 ; i<maxItrs ; ++i)
	{
		unsigned int listIdx(0), maxIdx(maxItrs-i), swaps(0);

		for(itr.CurrentNode = First ; listIdx < maxIdx ; ++listIdx)
		{
#if defined(INST_TRACKING_THREAD_TEST)
			if(!itr.CurrentNode->Next)
				throw(std::exception());
#endif
			auto currNode(itr.CurrentNode), nextNode(currNode->Next);
			if (comparisonFunction(*nextNode->TrackedInstance, *currNode->TrackedInstance))
			{
				SwapWithNext(currNode);
				++swaps;
			}
			else
			{
				++itr;
			}
		}

		if(!swaps)
			break;
	}
}

template<typename T>
void List<T>::SwapWithNext(Tracker<T>* a)
{
	Tracker<T>* b(a->Next);
	Tracker<T>* tmpPrev(a->Prev);

	a->Prev = b;
	a->Next = b->Next;
	b->Prev = tmpPrev;
	b->Next = a;

	(b->Prev ? b->Prev->Next : First) = b;
	(a->Next ? a->Next->Prev : Last) = a;
}


////////////////////////////////////////////////////////////////////////////
// Iterator - used to iterate through all tracked instances of a class

template<typename T>
struct Iterator
{
public:
	~Iterator() {HasMutex && List<T>::Unlock();}

	T* operator*() const {return CurrentNode->TrackedInstance;}
	T* operator->() const {return CurrentNode->TrackedInstance;}

	Iterator<T>& operator++()
	{
		CurrentNode = (CurrentNode ? CurrentNode->Next : CachedNextNode);
		CachedNextNode = nullptr;
		return *this;
	}

	bool operator==(const Iterator<T>& other) const { return CurrentNode == other.CurrentNode; }
	bool operator==(const T* other) const {return CurrentNode->TrackedInstance == other;}
	bool operator==(const T& other) const {return CurrentNode->TrackedInstance == &other;}

	template<typename Other>
	bool operator!=(const Other& other) const { return !(*this == other); }

	T* RemoveCurrentNode();

private:
	Iterator()
		: CurrentNode(nullptr)
		, CachedNextNode(nullptr)
		, HasMutex(false)
	{
	}

	Iterator(Tracker<T>* node, bool ownsMutex)
		: CurrentNode(node)
		, CachedNextNode(nullptr)
		, HasMutex(ownsMutex)
	{
	}

	static Iterator<T> EmptyIterator;

	Tracker<T>* CurrentNode;
	Tracker<T>* CachedNextNode;
	bool HasMutex;

	friend List<T>;
};

template<typename T> 
Iterator<T> Iterator<T>::EmptyIterator;

template<typename T>
T* Iterator<T>::RemoveCurrentNode()
{
	CachedNextNode = CurrentNode->Next;
	T* ret = CurrentNode->TrackedInstance;
	List<T>::Remove(CurrentNode, !HasMutex);
	CurrentNode = nullptr;
	return ret;
}

} // namespace Instance
} // namespace RTM

#undef INST_TRACKING_USING_STD_MUTEX