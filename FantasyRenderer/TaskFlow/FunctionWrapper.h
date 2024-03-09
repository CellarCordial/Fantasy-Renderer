#pragma once
#include <memory>

class FunctionWrapper
{
    struct ImplBase
    {
        virtual void Call() = 0;
        virtual ~ImplBase() = default;
    };

    template <typename F>
    struct ImplType : ImplBase
    {
        explicit ImplType(F&& InFunc) noexcept : Func(InFunc) {}
        void Call() override { Func(); }
        
        F Func;
    };
    
public:
    FunctionWrapper() = default;
    ~FunctionWrapper() = default;

    template <typename F, typename = std::enable_if_t<std::is_rvalue_reference_v<F&&>>>
    FunctionWrapper(F&& InFunc) : Func(std::make_unique<ImplType<F>>(std::forward<F>(InFunc))) {}

    FunctionWrapper(const FunctionWrapper&) = delete;
    FunctionWrapper& operator=(const FunctionWrapper&) = delete;

    FunctionWrapper(FunctionWrapper&& rhs) noexcept
        : Func(std::move(rhs.Func))
    {
    }

    FunctionWrapper& operator=(FunctionWrapper&& rhs) noexcept
    {
        Func = std::move(rhs.Func);
        return *this;
    }

public:

    void operator()() const
    {
        Func->Call();
    }

private:
    std::unique_ptr<ImplBase> Func;
};
