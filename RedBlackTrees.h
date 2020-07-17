//
// Created by marios on 15/3/20.
//

#ifndef REDBLACKTREES_H
#define REDBLACKTREES_H

#include <cstdio>
#include <zconf.h>

namespace exercise3namespace
{
    template<typename T>
    class RedBlackTree{
    private:
        using uint = unsigned int;
        enum Color{
            RED,
            BLACK
        };

        class GraphNode{
        public:
            GraphNode* _parent;
            GraphNode* _leftChild;
            GraphNode* _rightChild;
            Color _color;
            uint _key[3];
            GenericLinkedList<T*> _elementList;

            bool operator<(const GraphNode& rightNode) const
            {
                if (rightNode._key[2] > this->_key[2]) return true;
                if (rightNode._key[2] < this->_key[2]) return false;
                if (rightNode._key[1] > this->_key[1]) return true;
                if (rightNode._key[1] < this->_key[1]) return false;
                return rightNode._key[0] > this->_key[0];
            }

            bool operator<(const uint date[3]) const
            {
                if (date[2] > this->_key[2]) return true;
                if (date[2] < this->_key[2]) return false;
                if (date[1] > this->_key[1]) return true;
                if (date[1] < this->_key[1]) return false;
                return date[0] > this->_key[0];
            }

            bool operator>(const uint date[3]) const
            {
                if (date[2] > this->_key[2]) return false;
                if (date[2] < this->_key[2]) return true;
                if (date[1] > this->_key[1]) return false;
                if (date[1] < this->_key[1]) return true;
                return date[0] < this->_key[0];
            }

            bool operator<=(const uint date[3]) const
            {
                if (date[2] > this->_key[2]) return true;
                if (date[2] < this->_key[2]) return false;
                if (date[1] > this->_key[1]) return true;
                if (date[1] < this->_key[1]) return false;
                return date[0] >= this->_key[0];
            }

            bool operator>=(const uint date[3]) const
            {
                if (date[2] > this->_key[2]) return false;
                if (date[2] < this->_key[2]) return true;
                if (date[1] > this->_key[1]) return false;
                if (date[1] < this->_key[1]) return true;
                return date[0] <= this->_key[0];
            }

            bool operator==(const GraphNode& rightNode) const
            {
                if (rightNode._key[2] != this->_key[2]) return false;
                if (rightNode._key[1] != this->_key[1]) return false;
                return rightNode._key[0] == this->_key[0];
            }

            bool operator==(const uint date[3])
            {
                if (date[2] != this->_key[2]) return false;
                if (date[1] != this->_key[1]) return false;
                return date[0] == this->_key[0];
            }

            GraphNode():
                _parent(nullptr),
                _leftChild(nullptr),
                _rightChild(nullptr),
                _color(BLACK)
            {}

        };

        GraphNode* _root;
        bool deleteElements;
        bool LeftRotate(GraphNode* x);
        bool RightRotate(GraphNode* x);
        bool Insert(GraphNode* z);
        bool InsertFixup(GraphNode* z);
        GraphNode* SearchByDate(const uint date[3]);
        void GetNodesInAList(GenericLinkedList<GraphNode*>&, GraphNode* node);
        void RangeGetInternal(GenericLinkedList<T*>& elementList, GraphNode* node, const uint* date1, const uint* date2);
    public:

        bool InsertElement(uint date[3], T* elmenent);
        explicit RedBlackTree(bool deleteEl = true):
            deleteElements(deleteEl),
            _root(nullptr)
        {}
        GenericLinkedList<T*>* RangeGet(const uint date1[3], const uint date2[3]);
        ~RedBlackTree();
    };

    template<typename T>
    bool RedBlackTree<T>::LeftRotate(GraphNode* x)
    {
        if (!x) return false;
        GraphNode* y = x->_rightChild;
        if (!y) return false;
        x->_rightChild = y ->_leftChild;
        if (y->_leftChild) y->_leftChild->_parent = x;
        y->_parent = x->_parent;
        if (!x->_parent) _root = y;
        else if (x == x->_parent->_leftChild)
            x->_parent->_leftChild = y;
        else x->_parent->_rightChild = y;
        y->_leftChild = x;
        x->_parent = y;
        return true;
    }

    template<typename T>
    bool RedBlackTree<T>::RightRotate(GraphNode* x)
    {
        if (!x) return false;
        GraphNode* y = x->_leftChild;
        if (!y) return false;
        x->_leftChild = y ->_rightChild;
        if (y->_rightChild) y->_rightChild->_parent = x;
        y->_parent = x->_parent;
        if (!x->_parent) _root = y;
        else if (x == x->_parent->_leftChild)
            x->_parent->_leftChild = y;
        else x->_parent->_rightChild = y;
        y->_rightChild = x;
        x->_parent = y;
        return true;
    }

    template<typename T>
    bool RedBlackTree<T>::Insert(GraphNode *z)
    {
        if (!z) return false;
        GraphNode* y = nullptr;
        GraphNode* x = _root;
        while (x)
        {
            y = x;
            if (*z < *x) x = x->_leftChild;
            else x = x->_rightChild;
        }
        z->_parent = y;
        if (!y) _root = z;
        else if (*z < *y) y->_leftChild = z;
        else y->_rightChild = z;
        z->_leftChild = nullptr;
        z->_rightChild = nullptr;
        z->_color = RED;
        InsertFixup(z);
        return true;
    }

    template<typename T>
    bool RedBlackTree<T>::InsertFixup(GraphNode *z)
    {
        if (!z) return false;
        while (z->_parent && z->_parent->_color == RED)
        {
            if (z->_parent == z->_parent->_parent->_leftChild)
            {
                GraphNode *y = z->_parent->_parent->_rightChild;
                if (y && y->_color == RED)
                {
                    z->_parent->_color = BLACK;
                    y->_color = BLACK;
                    z->_parent->_parent->_color = RED;
                    z = z->_parent->_parent;
                }
                else if (z == z->_parent->_rightChild)
                {
                    z = z->_parent;
                    LeftRotate(z);
                }
                else if (z == z->_parent->_leftChild){
                    z->_parent->_color = BLACK;
                    z->_parent->_parent->_color = RED;
                    RightRotate(z->_parent->_parent);
                }
            }
            else{
                GraphNode *y = z->_parent->_parent->_leftChild;
                if (y && y->_color == RED)
                {
                    z->_parent->_color = BLACK;
                    y->_color = BLACK;
                    z->_parent->_parent->_color = RED;
                    z = z->_parent->_parent;
                }
                else if (z == z->_parent->_leftChild)
                {
                    z = z->_parent;
                    RightRotate(z);
                }
                else if (z == z->_parent->_rightChild){
                    z->_parent->_color = BLACK;
                    z->_parent->_parent->_color = RED;
                    LeftRotate(z->_parent->_parent);
                }
            }
        }
        _root->_color = BLACK;
        return true;
    }

    template<typename T>
    typename RedBlackTree<T>::GraphNode* RedBlackTree<T>::SearchByDate(const uint date[3])
    {
        GraphNode* x = _root;
        while (x)
        {
            if (*x == date) break;
            if (*x > date) x = x->_leftChild;
            else x = x->_rightChild;
        }
        return x;
    }

    template<typename T>
    bool RedBlackTree<T>::InsertElement(uint date[3], T* elmenent)
    {
        GraphNode* alreadyPresentNodeDate = SearchByDate(date);
        if (alreadyPresentNodeDate)
        {
            alreadyPresentNodeDate->_elementList.Append(elmenent);
        }
        else{
            GraphNode* node = new GraphNode();
            node->_elementList.Append(elmenent);
            for (uint i = 0; i < 3; i++)
            {
                node->_key[i] = date[i];
            }
            Insert(node);
        }
        return true;
    }

    template<typename T>
    void RedBlackTree<T>::GetNodesInAList(GenericLinkedList<GraphNode*>& list, GraphNode *node)
    {
        if (node)
        {
            GetNodesInAList(list, node->_leftChild);
            GetNodesInAList(list, node->_rightChild);
            list.Append(node);
        }
    }

    template<typename T>
    RedBlackTree<T>::~RedBlackTree()
    {
        GenericLinkedList<GraphNode*> nodeList;
        GetNodesInAList(nodeList, _root);
        for (GenericLinkedListIterator<GraphNode*> nodeIterator(nodeList); !nodeIterator.End(); nodeIterator++)
        {
            if (deleteElements)
            {
                T **ppElement;
                GraphNode *node = *(nodeIterator.GetElementOnIterator());
                for (GenericLinkedListIterator<T *> nodeIteratorGraphNode(node->_elementList); !nodeIteratorGraphNode.End(); nodeIteratorGraphNode++) {
                    ppElement = nodeIteratorGraphNode.GetElementOnIterator();
                    delete *ppElement;
                }
            }
            delete *(nodeIterator.GetElementOnIterator());
        }
    }

    template<typename T>
    void RedBlackTree<T>::RangeGetInternal(GenericLinkedList<T*>& elementList, GraphNode* node, const uint* date1, const uint* date2)
    {
        if (node != nullptr && *node >= date1 && *node <= date2)
        {
            T** ppElenent;
            for (GenericLinkedListIterator<T*> nodeIterator(node->_elementList); !nodeIterator.End(); nodeIterator++) {
                ppElenent = nodeIterator.GetElementOnIterator();
                elementList.Append(*ppElenent);
            }
            RangeGetInternal(elementList, node->_leftChild, date1, date2);
            RangeGetInternal(elementList, node->_rightChild, date1, date2);
        }
        else if (node != nullptr && *node < date1){
            RangeGetInternal(elementList, node->_rightChild, date1, date2);
        }
        else if (node != nullptr && *node > date2){
            RangeGetInternal(elementList, node->_leftChild, date1, date2);
        }
    }

    template<typename T>
    GenericLinkedList<T*>* RedBlackTree<T>::RangeGet(const uint date1[3], const uint date2[3])
    {
        GenericLinkedList<T*>* elementList = new GenericLinkedList<T*>;
        RangeGetInternal(*elementList, _root, date1, date2);
        return elementList;
    }
}

#endif