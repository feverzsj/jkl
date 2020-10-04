// template argument deduction of forwarding reference
// 
// template argument deduction happens in function template argument deduction, and when auto is used (except in trailing return type declaration, and decltype(auto)); 
// 
// template<class T>
// void fun(T&& t)
// {
//     decltype(t);
// }
// 
// suppose A is a possibly cv-qualified, non-reference, non-void type
//                                             //                                              std::forward<     T     >(t) / static_cast<    T&&    >(t)
// A a;                                        //  expr  | T/auto | T&&/auto&& | decltype(t) | std::forward<decltype(t)>(t) / static_cast<decltype(t)>(t)
// fun(      a     ); auto&& t =       a     ; // lvalue |   A&   |    A&      |    A&       |     A&
// fun(std::move(a)); auto&& t = std::move(a); // xvalue |   A&&  |    A&&     |    A&&      |     A&&
// fun(     A()    ); auto&& t =      A()    ; // prvalue|   A    |    A&&     |    A&&      |     A&&
//                                             // resulted
//                                             // temporary

// The reference collapsing rule:
// in all reference to reference combinations, i.e., T&/T&&, when T is A&/A&&,
// ONLY rvalue reference to rvalue reference collapses to rvalue reference (A&& && -> A&&),
// all other combinations form lvalue reference.
// so when you want to perfect forward a reference: static_cast<T&&>(t).
// 
// NOTE: for T cv-qualifiers& or T cv-qualifiers&&, if T is reference type, the cv-qualifiers are simply dropped.
// NOTE: add_r/lvalue_reference<> follows reference collapsing rules, so they just add && or & to type and collapsing rule decided the type.
//       std::declval use add_rvalue_reference so user can also declare lvalue reference with std::declval<A&>();
// NOTE: if a function type has a cv- or ref-qualifier, it is not possible to create the referenced type. add_r/lvalue_reference<> will return as is.
// 
// https://stackoverflow.com/questions/61823137/why-does-a-const-reference-to-a-reference-lose-its-constness
// https://stackoverflow.com/questions/57856978/add-lvalue-reference-add-rvalue-reference-and-cv-qualified-type
// https://en.cppreference.com/w/cpp/language/reference#Reference_collapsing

// Value category:
// lvalue : a value with identity/name, e.g.: function parameter in function body is lvaue, which will bind to lvalue reference (A&) in template argument deduction of forwarding reference, so std::forward requires explicity specify it's type
// xvalue : std::move(a)/static_cast<A&&>(a)/std::move(a).member..., bind to rvalue reference (A&&) in template argument deduction of forwarding reference.
// prvalue: in general, is expr of non reference complete type.
//          it can be used to initialize object of same type, the effect is as if prvalue became the object. That's the rational of guaranteed copy elision.
//          when prvalue is used in other scenes, which is without initializing any named object of same type, a temporary of same type is initialized from it. (temporary materialization)
//          temporaries bind to rvalue reference (A&&) in template argument deduction of forwarding reference.
// 
// NOTE: an expression's value categories and it's type are different concepts,
//       a lvalue can be any type: A a; A& a1 = a; A&& a2 = std::move(a); a, a1, a2 are all lvalue with different types.
// 
// https://en.cppreference.com/w/cpp/language/value_category
// https://jonasdevlieghere.com/guaranteed-copy-elision/
// https://stackoverflow.com/questions/38043319/how-does-guaranteed-copy-elision-work
// https://en.cppreference.com/w/cpp/language/implicit_conversion#Temporary_materialization
// https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/cpp-value-categories


// if you want to return a local object from a function:
//
//   auto foo() { T t; ...; [co_]return t; } // DON't use [co_]return std::move(t). "automatic move from local variables
//                                           // and parameters(since C++11)" assures move ctor will be used if valid.
//                                           // Or even better, the compiler may apply NRVO to elide any move/copy,
//                                           // only if the return statement simply returns a local object.
//                                           // [ref]: https://en.cppreference.com/w/cpp/language/return#Notes

// if you want to release an class member:
//   class A
//   {
//      T t;
//   public:
//      
//      T release() { return std::move(t); }
//      
//      T&& bad_release() { return std::move(t); } // DON't use this
//      
//      // auto&& t = A().bad_release(); // dangling reference, also wrongly extended lifetime cause ~T() being called twice.
//      // ^^^^^^^^ compiler generated code(range based for/coroutines...), use forwarding reference frequently. Since it can
//      //          either perfect forwarding references or if right side is temporary, auto&& will bind to rvalue reference
//      //          which extends the temporary's lifetime.
//      //          [ref]: https://en.cppreference.com/w/cpp/language/lifetime
//   };


// decltype(e): yields the type of expression, without surprise
// 
//      int* p; decltype(*p) is int&, since operator* of pointer returns int&
//      int a, b; decltype(a = b) is int&, since operator= returns int&
//                decltype(a + b) is int, since operator+ returns int or prvalue
//      decltype("s") is char const(&)[2], string literal is lvalue, while all other literals are prvalues.
//              https://stackoverflow.com/questions/30293262/why-does-decltype-on-a-string-literal-not-yield-an-array-type
// 
// decltype((e)) : equal to decltype(e)& which is always a lvalue reference, according to collapsing rules.
// 
// decltype(auto) : yields the type of expression which auto is placeholder for.
// 
// 
// https://en.cppreference.com/w/cpp/language/decltype
// https://en.cppreference.com/w/cpp/language/auto
// https://stackoverflow.com/questions/17241614/what-expressions-yield-a-reference-type-when-decltype-is-applied-to-them



// requires expression
// parameter list:
//  requires(T t){}
//      first, T is directly replaced with actual type of T. Then the parameter list act as function parameter list(not template function's).
//      so, if T is an array type, t will be a pointer. You need use requires(T& t).
//  according to reference collapsing rule:
//      requires(T&  t){} // decltype(t)/T& is always lvalue reference.
//      requires(T&& t){} // decltype(t)/T&& is rvalue reference only if T is rvalue reference or non-reference type. Otherwise it's lvalue reference.
// to force rvalue reference:
//      requires(std::remove_reference_t<T>&& t){}
// to force reference to cv-qualified type:
//      requires(std::remove_reference_t<T> cv-qualifiers& t){}



// the '...' or 'Init-expr' on which side, implies the forced associativity of operator
// (which is force by parentheses, so it just looks like the operator's associativity may be changed)
// 
// ([I op] ...  op E ) is left associativity, evaluated from left side
// ( E op  ... [op I]) is right associativity, evaluated from right side
// 
// ([I op] ...  op E ) => (((([I op] E1) op E2) op ...) op EN)
// ( E op  ... [op I]) => (E1 op (... op (EN−1 op (EN [op I]))))
// 
// https://en.cppreference.com/w/cpp/language/fold



// trailing return type
// ignore the auto on head, just replace it with tailer
// auto fun()->int
// auto fun()->decltype(expr)
// auto fun()->decltype(auto)
// auto fun()->auto
// auto fun()->auto&


// lambda return type
// 
// [ captures ] ( params ) -> trailing-return-type { body }
// 
// when trailing-return-type is omitted, the return type is deduced from
// return statements as if for a function whose return type is declared auto.
// so the only way to around it is to use trailing return type
// 


// memory ordering:
// https://en.cppreference.com/w/cpp/atomic/memory_order
// 
// Release-Acquire ordering:
// 
//       std::atomic_xxx atomic;
//       
//    release-store threads:
//       
//       // * Writes happen before this release-store in same thread,
//       //   will be visible in other threads when after acquire-load on same atomic, if the release-store happens before the acquire-load.
//       // * Read and write instructions before the release-store in same thread,
//       //   won't be reordered after it, for both compiler and cpu.
//    
//       atomic.store(std::memory_order_release);
//       
//    acquire-load threads:
//    
//       atomic.load(std::memory_order_acquire);
//       
//       // * Writes happen before release-storing the same atomic in other threads,
//       //   are visible after this acquire-load, if the release-store happens before the acquire-load.
//       // * Read and write instructions after this acquire-load in same thread,
//       //   won't be reordered before it, for both compiler and cpu.
//
// Read-Modify-Write:
//      * Atomic operations involving both store/read and load/write,
//        if tagged as release(for store) or acquire(for load),
//        the other part of load or store is tagged as relaxed.
//      * acq_rel would tag release(for store) and acquire(for load):
//        If the same code is executed in multiple threads, and
//        only one Read-Modify-Write on same atomic is executed,
//        tag it as acq_rel, would give Release-Acquire ordering across threads.
// 
// Sequentially-consistent ordering:
//      * Same as Release-Acquire ordering, plus a single total order exists
//        in which all threads observe all modifications in the same order.
//      * It is useful when multiple atomics are involved and Release-Acquire
//        is difficult to achieve the expected ordering.
//      * For Read-Modify-Write, seq_cst is acq_rel plus a single total order
//        across all involved threads.




// one may use rethrow to catch the specific exception in the handler
// try {
//     throw;
// }
// catch(Exception& e)
// {
//     ...
// }
// or std::current_exception(), std::rethrow_exception()


// Customization Point dispatch:
// Unqualified call on non-member function(also including friend function defined inside class) is a typical customization point,
// but can be dangerous, especially for common names, since unqualified call with ADL essentially puts all overloads in global namespace.
// A better solution is using a functor as Customization Point Object. Since it's not a real function, it won't trigger ADL.
// When CPO is invoked, it calls jkl_customize(jkl::cp_xxx/decltype(jkl::xxx), ...), which uses ADL with minimal drawbacks.
// NOTE: use qualified call if a non-member function is not supposed to be a customization point, (foo)(), ::foo(), ns::foo(), ::ns::foo().
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1895r0.pdf
// https://github.com/facebookexperimental/libunifex/blob/master/include/unifex/tag_invoke.hpp



// coroutine:

// any function(free, member, lambda) where its BODY contains any of co_return, co_await, or co_yield, is a coroutine.
// such function is transformed to something roughly like the following:

    RetType func(Args... args)
    {
        using PromiseType = std::coroutine_traits<RetType, Args...>::promise_type;

        // All local variables are actually constructed inside this buffer.
        // The arguments to "func()" will also be copied into this buffer.
        // The allocation may be elided if the compiler can prove it safe.
        // "operator new" will first be looked up in PromiseType, then globally.
        // If the found name is not callable with "args...", it will try again
        // without passing them. This is known as "parameter preview" and allows
        // things like passing the allocator that should be used.
        void* coroFrame;
        size_t coroFrameSize = /*...*/;
        if constexpr(has PromiseType::get_return_object_on_allocation_failure)
        {
            coroFrame = operator new(coroFrameSize, std::nothrow, args...);

            if(!coroFrame)
                return PromiseType::get_return_object_on_allocation_failure();
        }
        else
        {
            coroFrame = operator new(coroFrameSize, args...);
        }

        // Will fall back to "PromiseType()" if it can't
        // be constructed like this. Note that the promise object
        // will be allocated in the coroFrame buffer and will live
        // as long as the coroutine.
        auto promise = PromiseType(args...);
        auto coro    = std::coroutine_handle<PromiseType>::from_promise(promise);

        // This is the value that is actually returned to the caller
        // of "func()". It is unspecified whether this conversion
        // happens before calling "initial_suspend()" or the first
        // time the coroutine actually suspends or returns, which
        // is when "func()" returns to its caller. If it is called
        // late, you can use that to optimize for cases where the
        // coro completes without ever suspending.
        auto protoReturnValue = promise.get_return_object(); // first constructed in coroFrame
        RetType actualReturnValue = std::move(protoReturnValue); // then move out as return value of the function.

        // This may move inside the try block in a future version of the TS.
        co_await promise.initial_suspend();

        try
        {
            // This will contain usually a call to "promise.return_void()"
            // or "promise.return_value()". See co_return below.
            BODY;

            if constexpr(promise has return_void())
            {
                // Make rolling off the end of BODY well defined for
                // coros that (logically) return "void". For other
                // kinds of coros this is UB that your compiler should
                // warn about, just like in normal functions.
                co_return;
            }
        }
        catch(...)
        {
            // This can get the exception using either "throw;"
            // or "std::current_exception()".
            promise.unhandled_exception();
        }

        co_await promise.final_suspend();
        // It is common for coroutines to never be resumed from
        // this call if they suspend here. In that case, the
        // following line will never execute, and something
        // else must free the storage by calling "destory()"
        // on the coroutine handle.

        // Will fallback to single-argument form.
        operator delete(coroFrame, coroFrameSize);
    }


// The semantics of a "co_await <expr>" can be roughly summarized by the following pseudo-code:

    [&]()
    {
        // Note: The await_transform operation is only done on
        // users' co_awaits, not the implicit co_awaits on the
        // returns from yield_value, initial_suspend, and
        // final_suspend, because the promise is already involved
        // in those.
        auto&& Awaiter awaiter =
            //operator co_await(    // if <expr> or await_transform(<expr>) has the operator overloading
                //promise.await_transform(  // if supplied in the promise
                    <expr>  // if no transform or operator co_await overloading is applied, <expr> is the awaiter, otherwise it's awaitable.
                //)
            //)
            ;

        if(! awaiter.await_ready())
        {
            <suspend the coro>

            using SuspendRet = decltype(awaiter.await_suspend(coro));

            if constexpr(is_void<SuspendRet>())
            {
                awaiter.await_suspend(coro);
                <return to the caller who resumed this coro>;
            }
            else if constexpr(is_bool<SuspendRet>())
            {
                if(awaiter.await_suspend(coro))
                    <return to the caller who resumed this coro>;
                else
                    goto <resume point>; // coro is immediately resumed
            }
            else
            {
                auto dest = awaiter.await_suspend(coro);

                static_assert(dest is coroutine_handle<...>);

                if(dest == noop_coroutine_handle)
                    <return to the caller who resumed this coro>;
                else
                    <pop the stack frame and call dest.resume()>;
            }
        }

        <coro resume point>
        return awaiter.await_resume(); // May return void
    }();




// in the case of jkl's design:

    atask<T> func(Args... args)
    {
        using PromiseType = atask<T>::promise_type();

        // allocate coro frame buffer
        void* coroFrame;
        size_t coroFrameSize = /*...*/;

        auto promise = PromiseType();

        atask<T> task = promise.get_return_object();
        // task and promise have member to ref each other,
        // when task.start_deatch() or promise.~dtor() is called, the ref will be cleared.

        co_await promise.initial_suspend():
        // promise.frame().coro = this coro
        // the coro is suspended, see "co_await task" below

        try
        {
            //BODY

            co_await awaiter; // see below

            co_return ...;
        }
        catch(...)
        {
            promise.unhandled_exception();
        }

        // co_await promise.final_suspend():


    }

    atask<U> parent_coro_func()
    {
        atask<T> task = func(...); // the coro is suspened on initial_suspend() and hold by task.

        co_await task; // unless the task is started via start_detach(), task owns its coro.

        co_await asyncOp; // see below

        co_return ...
    }

    // co_await task:
    [&]()
    {
        // task.await_ready() returns false
        <suspend parent_coro>;

        // task.await_suspend(parent_coro):
        if(parent_promise.stop_forced())
        {
            exception = boost::system::system_error(asio::error::operation_aborted);
            <parent_coro immediately resumed, and task.await_resume() is called with exception thrown>
        }
        else
        {
            // parent_promise get access to task's stoppable
            task.connect_stopper(parent_promise);
            // task's coro_frame is pushed on parent's coro frame stack
            parent_coro_frame_stack.push(task_coro_frame);

            task_coro.resume(); // start run func body.
            
            // when body is co_return-ed
            // and final_suspend point is reached(see above),
            // the parent_coro is resumed:
            <parent_coro resume point>;
            // awaiter.await_resume():
            task.clear_stopper();
            return task.result;
        }
    }();


    // co_await asyncOp:
    [&]()
    {
        // asyncOp.await_ready() returns false
        <suspend parent_coro>;

        // asyncOp.await_suspend(parent_coro):
        if(parent_promise.stop_forced())
        {
            exception = boost::system::system_error(asio::error::operation_aborted);
            <parent_coro immediately resumed, and task.await_resume() is called with exception thrown>
        }
        else
        {
            // parent_promise get access to asyncOp's stoppable
            asyncOp.set_stopper(parent_promise);

            <schedule completion handle>;
            // the whole coro frame stack is also held by completion handle
            // until it is invoked, so, in case the handle is destroyed
            // before invocation, everything is destroyed properly.
            // Inside invocation, parent_coro is resumed.
            
            <parent_coro resume point>;
            // awaiter.await_resume():
            asyncOp.clear_stopper();
            return asyncOp.result;
        }
    }();


    // stop mechanism:
    // any awaiter_stopper awaiter should set to its parent_coro's promise, when it is co_await-ed, and clear itself when await_resume()ed.
    // thus, forms a chain of notify.
    // when task.[promise.]stop(lv) is called, a flag is set, and if a awaiter_stopper awaiter is set, its stop(lv) is called.
    // future co_await-s may immediately return with asio::error::operation_aborted, if the stop_lv fits its requirement.
