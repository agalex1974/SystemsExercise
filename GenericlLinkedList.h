//
// Created by marios on 15/3/20.
//

#ifndef GENERIKLLINKEDLIST_H
#define GENERIKLLINKEDLIST_H

namespace exercise3namespace
{
    template<typename T>
    class GenericLinkedListIterator;

    template<typename T>
    class GenericLinkedList {
        struct GenericNodeElement {
            T dataElement;
            GenericNodeElement *next;
        };
        using uint = unsigned int;
        GenericNodeElement* _head;
        GenericNodeElement* _tail;
    public:
        GenericLinkedList();
        bool IsListEmpty() const;
        bool Append(const T &element);
        GenericLinkedList(const GenericLinkedList&) = delete; //copy contructor not implemented
        GenericLinkedList& operator=(const GenericLinkedList&) = delete; //assignment operator not implemented

        ~GenericLinkedList();

        friend class GenericLinkedListIterator<T>;
    };

    template<>
    class GenericLinkedList<PatientEntry*> {
        struct GenericNodeElement {
            PatientEntry* dataElement;
            GenericNodeElement *next;
            /*~GenericNodeElement(){
                delete dataElement;
            }*/
        };
        using uint = unsigned int;
        GenericNodeElement* _head;
        GenericNodeElement* _tail;
    public:
        GenericLinkedList();
        bool IsListEmpty() const;
        bool Append(PatientEntry* element, bool noRecordId = false);
        bool ErasePatientEntries();
        GenericLinkedList(const GenericLinkedList&) = delete; //copy contructor not implemented
        GenericLinkedList& operator=(const GenericLinkedList&) = delete; //assignment operator not implemented
        ~GenericLinkedList();
        friend class GenericLinkedListIterator<PatientEntry*>;
    };

    template<typename T>
    class GenericLinkedListIterator{
    public:
        explicit GenericLinkedListIterator(const GenericLinkedList<T>& linkedList);
        T* GetElementOnIterator();
        GenericLinkedListIterator& operator++();
        const GenericLinkedListIterator operator++(int);
        void RewindIteraror();
        bool End();
        static T* ReturnLast(const GenericLinkedList<T>& linkedList)
        {
            return &linkedList._tail->dataElement;
        }
    private:
        typename GenericLinkedList<T>::GenericNodeElement* _head;
        typename GenericLinkedList<T>::GenericNodeElement* _currentNode;
    };

    template<typename T>
    inline GenericLinkedListIterator<T>::GenericLinkedListIterator(const GenericLinkedList<T>& linkedList)
    {
        _currentNode = linkedList._head;
        _head = _currentNode;
    }

    template<typename T>
    inline T* GenericLinkedListIterator<T>::GetElementOnIterator()
    {
        if (_currentNode) return &_currentNode->dataElement;
        else return nullptr;
    }

    //++it
    template<typename T>
    inline GenericLinkedListIterator<T>& GenericLinkedListIterator<T>::operator++(){
        if (_currentNode) _currentNode = _currentNode->next;
        return *this;
    }

    //it++
    template<typename T>
    inline const GenericLinkedListIterator<T> GenericLinkedListIterator<T>::operator++(int){
        GenericLinkedListIterator copy = *this;
        ++(*this);
        return copy;
    }

    template<typename T>
    inline void GenericLinkedListIterator<T>::RewindIteraror()
    {
        _currentNode = _head;
    }

    template<typename T>
    inline bool GenericLinkedListIterator<T>::End()
    {
        return _currentNode == nullptr;
    }

    template<typename T>
    GenericLinkedList<T>::GenericLinkedList()
    {
        _head = nullptr;
        _tail = nullptr;
    }

    template<typename T>
    bool GenericLinkedList<T>::IsListEmpty() const {
        return _head == nullptr;
    }

    template<typename T>
    bool GenericLinkedList<T>::Append(const T& element)
    {
        if (IsListEmpty()){
            GenericNodeElement* pElement = new GenericNodeElement();
            pElement->dataElement = element;
            pElement->next = nullptr;
            _head = pElement;
            _tail = _head;
        }
        else{
            GenericNodeElement* previousNode = _tail;
            GenericNodeElement* pElement = new GenericNodeElement();
            pElement->dataElement = element;
            pElement->next = nullptr;
            previousNode->next = pElement;
            _tail = pElement;
        }
        return true;
    }

    template<typename T>
    GenericLinkedList<T>::~GenericLinkedList()
    {
        GenericNodeElement* currentNode = _head;
        while (currentNode)
        {
            GenericNodeElement* toDeleteNode = currentNode;
            currentNode = currentNode->next;
            delete toDeleteNode;
        }
    }
}

#endif
