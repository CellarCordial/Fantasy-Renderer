#pragma once
#include "ConcurrentList.h"
#include "../Utility/AlignUtil.h"


constexpr size_t BLOCK_SIZE_MIN = 2; 


class ConcurrentBuddyAllocator
{
    struct Block
    {
        friend bool operator > (const Block& lhs, const Block& rhs) noexcept { return lhs.Start > rhs.Start; }
        friend bool operator < (const Block& lhs, const Block& rhs) noexcept { return lhs.Start < rhs.Start; }
        friend bool operator == (const Block& lhs, const Block& rhs) noexcept { return lhs.Start == rhs.Start && lhs.Size == rhs.Size; }

        size_t Start;
        size_t Size;    // 总是为2的幂次方大小
    };

    struct Node
    {
        Node() = default;
        Node(const Block& InBlock) :Data(std::make_shared<Block>(InBlock)) {}

        Node* Next = nullptr;
        Node* Prev = nullptr;
        std::mutex Mutex;
        std::shared_ptr<Block> Data;
    };

public:
    CLASS_NO_COPY(ConcurrentBuddyAllocator)
    
    ConcurrentBuddyAllocator(size_t InMaxSize, size_t InAlignSize = 2) : MaxSize(AlignPow2(InMaxSize, InAlignSize)), AlignSize(InAlignSize)
    {
        Head.Data = std::make_shared<Block>(INVALID_SIZE_64, INVALID_SIZE_64);
        Head.Next = new Node(Block{ 0, MaxSize });
        Head.Next->Prev = &Head;
    }
    ~ConcurrentBuddyAllocator() noexcept { Clear(); }

public:
    bool TryAllocate(size_t* OutAddress, size_t InSize)
    {
        std::unique_lock HeadLock(Head.Mutex);
        if (Head.Next == nullptr) return false;

        *OutAddress = Allocate(std::move(HeadLock), AlignPow2(InSize, AlignSize));
        if (*OutAddress == INVALID_SIZE_64) return false;
        return true;
    }

    bool TryFree(size_t InAddress, size_t InSize)
    {
        const Block InBlock{ InAddress, AlignPow2(InSize, AlignSize) };
        if (InAddress + InSize > MaxSize || AlreadyFree(InBlock)) return false;
        
        Block BuddyBlock(GetBuddyBlock(InBlock));

        std::unique_lock CurrentNodeLock(Head.Mutex);
        Node* CurrentNode = &Head;

        bool Finished = false;
        while (Node* NextNode = CurrentNode->Next)
        {
            std::unique_lock NextNodeLock(NextNode->Mutex);

            if (BuddyBlock == *NextNode->Data)
            {
                CurrentNodeLock.unlock();
                MergeBuddyBlock(NextNode, std::move(NextNodeLock));
                Finished = true; break;
            }
            else if (BuddyBlock < *NextNode->Data)
            {
                Node* NewNode = new Node(InBlock);
                NewNode->Next = NextNode;
                NewNode->Prev = CurrentNode;
                CurrentNode->Next = NewNode;
                NextNode->Prev = NewNode;
                Finished = true; break;
            }
            CurrentNodeLock.unlock();
            CurrentNode = NextNode;
            CurrentNodeLock = std::move(NextNodeLock);
        }

        if (!Finished)
        {
            Node* NewNode = new Node(InBlock);
            NewNode->Prev = CurrentNode;
            CurrentNode->Next = NewNode;
        }
        return true;
    }

    void Clear()
    {
        Node* CurrentNode = &Head;
        std::lock_guard CurrentNodeLock(CurrentNode->Mutex);

        while (Node* NextNode = CurrentNode->Next)
        {
            std::unique_lock NextNodeLock(NextNode->Mutex);
            CurrentNode->Next = NextNode->Next;
            if (CurrentNode->Next)
            {
                CurrentNode->Next->Prev = CurrentNode;
            }

            NextNode->Data.reset();
            NextNodeLock.unlock();
            delete NextNode;
            NextNode = nullptr; 
        }
    }

    void PrintAll()
    {
        Node* CurrentNode = &Head;
        std::unique_lock CurrentNodeLock(CurrentNode->Mutex);

        while (Node* NextNode = CurrentNode->Next)
        {
            std::unique_lock NextNodeLock(NextNode->Mutex);
            CurrentNodeLock.unlock();
            printf_s("(%llu, %llu)\n", NextNode->Data->Start, NextNode->Data->Size);

            CurrentNode = NextNode;
            CurrentNodeLock = std::move(NextNodeLock);            
        }
    }
    
private:
    static Block GetBuddyBlock(const Block& InBlock)
    {
        Block BuddyBlock(InBlock);
        if ((InBlock.Start / InBlock.Size) % 2 == 0)
        {
            BuddyBlock.Start += InBlock.Size;
        }
        else
        {
            BuddyBlock.Start -= InBlock.Size;
        }
        return BuddyBlock;
    }

    static Block GetCompleteBlock(const Block& InBlock)
    {
        Block CompleteBlock;
        if ((InBlock.Start / InBlock.Size) % 2 == 0)
        {
            CompleteBlock = { InBlock.Start, InBlock.Size << 1 };
        }
        else
        {
            CompleteBlock = { InBlock.Start - InBlock.Size, InBlock.Size << 1 };
        }
        return CompleteBlock;
    }

    bool AlreadyFree(const Block& InBlock)
    {
        std::unique_lock CurrentNodeLock(Head.Mutex);

        Node* CurrentNode = &Head;

        while (Node* NextNode = CurrentNode->Next)
        {
            std::unique_lock NextNodeLock(NextNode->Mutex);
            CurrentNodeLock.unlock();

            if (InBlock == *NextNode->Data) return true;
            
            CurrentNode = NextNode;
            CurrentNodeLock = std::move(NextNodeLock);
        }
        return false;
    }

    static void MergeBuddyBlock(Node* InNode, std::unique_lock<std::mutex> InNodeLock)
    {
        std::unique_lock CurrentNodeLock(std::move(InNodeLock));

        Node* CurrentNode = InNode;
        
        Block BuddyBlock = *CurrentNode->Data;
        Block FreeBlock;

        while (true)
        {
            FreeBlock = GetCompleteBlock(BuddyBlock);
            BuddyBlock = GetBuddyBlock(FreeBlock);

            if (CurrentNode->Next && *CurrentNode->Next->Data == BuddyBlock)
            {
                CurrentNode->Data.reset();

                Node* PrevNode = CurrentNode->Prev;
                Node* NextNode = CurrentNode->Next;
                
                std::unique_lock NextNodeLock(NextNode->Mutex);
                NextNode->Prev = PrevNode;

                if (PrevNode)
                {
                    std::lock_guard PrevNodeLock(PrevNode->Mutex);
                    PrevNode->Next = NextNode;
                }
                
                CurrentNodeLock.unlock();
                delete CurrentNode;

                CurrentNode = NextNode;
                CurrentNodeLock = std::move(NextNodeLock);
            }
            else if (CurrentNode->Prev && *CurrentNode->Prev->Data == BuddyBlock)
            {
                CurrentNode->Data.reset();

                Node* PrevNode = CurrentNode->Prev;
                Node* NextNode = CurrentNode->Next;

                std::unique_lock PrevNodeLock(PrevNode->Mutex);
                PrevNode->Next = NextNode;

                if (NextNode)
                {
                    std::lock_guard NextNodeLock(NextNode->Mutex);
                    NextNode->Prev = PrevNode;
                }
                
                CurrentNodeLock.unlock();
                delete CurrentNode;

                CurrentNode = PrevNode;
                CurrentNodeLock = std::move(PrevNodeLock);
            }
            else
            {
                break;
            }
        }

        *CurrentNode->Data = FreeBlock;
    }
    
    size_t Allocate(std::unique_lock<std::mutex> InHeadLock, size_t InAlignedSize)
    {
        std::unique_lock CurrentNodeLock(std::move(InHeadLock));
        
        Node* CurrentNode = &Head;

        size_t Level = 0;
        while (InAlignedSize <= MaxSize)
        {
            while (Node* NextNode = CurrentNode->Next)
            {
                std::unique_lock NextNodeLock(NextNode->Mutex);

                if (NextNode->Data->Size == InAlignedSize)
                {
                    const size_t Output = NextNode->Data->Start;
                    
                    size_t CurrentSize = InAlignedSize;
                    Node* OldNextNode = CurrentNode->Next;
                    CurrentNode->Next = OldNextNode->Next;
                    if (OldNextNode->Next)
                    {
                        OldNextNode->Next->Prev = CurrentNode;
                    }
                    for (size_t ix = 0; ix < Level; ++ix)
                    {
                        CurrentSize >>= 1;
                            
                        Node* NewNode = new Node(Block{ OldNextNode->Data->Start + CurrentSize, CurrentSize });
                        NewNode->Prev = CurrentNode;
                        NewNode->Next = CurrentNode->Next;
                        if (CurrentNode->Next)
                        {
                            CurrentNode->Next->Prev = NewNode;
                        }
                        CurrentNode->Next = NewNode;
                    }
                    OldNextNode->Data.reset();

                    NextNodeLock.unlock();
                    delete OldNextNode;
                    OldNextNode = nullptr;
                    
                    return Output;
                }
                else
                {
                    CurrentNodeLock.unlock();
                    CurrentNode = NextNode;
                    CurrentNodeLock = std::move(NextNodeLock);
                }
            }
            Level++;
            InAlignedSize <<= 1;

            std::unique_lock HeadLock(Head.Mutex);
            CurrentNodeLock.unlock();
            CurrentNode = &Head;
            CurrentNodeLock = std::move(HeadLock);
        }
        return INVALID_SIZE_64;
    }

    void PushFront(const Block& InBlock)
    {
        std::unique_lock HeadLock(Head.Mutex);

        Node* NewNode = new Node(InBlock);
        NewNode->Next = Head.Next;
        NewNode->Prev = &Head;
        Head.Next = NewNode;
    }

private:
    Node Head;
    size_t MaxSize;
    size_t AlignSize;
};
