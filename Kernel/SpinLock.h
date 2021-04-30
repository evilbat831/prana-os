#pragma once

// includes
#include <AK/Atomic.h>
#include <AK/Types.h>
#include <Kernel/Arch/x86/CPU.h>
#include <Kernel/Forward.h>

namespace Kernel {

template<typename BaseType = u32>
class SpinLock {
    AK_MAKE_NONCOPYABLE(SpinLock);
    AK_MAKE_NONMOVABLE(SpinLock);

public:
    SpinLock() = default;

    ALWAYS_INLINE u32 lock()
    {
        u32 prev_flags;
        Processor::current().enter_critical(prev_flags);
        while (m_lock.exchange(1, AK::memory_order_acquire) != 0) {
            Processor::wait_check();
        }
        return prev_flags;
    }

    ALWAYS_INLINE void unlock(u32 prev_flags)
    {
        VERIFY(is_locked());
        m_lock.store(0, AK::memory_order_release);
        Processor::current().leave_critical(prev_flags);
    }

    [[nodiscard]] ALWAYS_INLINE bool is_locked() const
    {
        return m_lock.load(AK::memory_order_relaxed) != 0;
    }

    ALWAYS_INLINE void initialize()
    {
        m_lock.store(0, AK::memory_order_relaxed);
    }

private:
    Atomic<BaseType> m_lock { 0 };
};

class RecursiveSpinLock {
    AK_MAKE_NONCOPYABLE(RecursiveSpinLock);
    AK_MAKE_NONMOVABLE(RecursiveSpinLock);

public:
    RecursiveSpinLock() = default;

    ALWAYS_INLINE u32 lock()
    {
        auto& proc = Processor::current();
        FlatPtr cpu = FlatPtr(&proc);
        u32 prev_flags;
        proc.enter_critical(prev_flags);
        FlatPtr expected = 0;
        while (!m_lock.compare_exchange_strong(expected, cpu, AK::memory_order_acq_rel)) {
            if (expected == cpu)
                break;
            Processor::wait_check();
            expected = 0;
        }
        m_recursions++;
        return prev_flags;
    }

    ALWAYS_INLINE void unlock(u32 prev_flags)
    {
        VERIFY(m_recursions > 0);
        VERIFY(m_lock.load(AK::memory_order_relaxed) == FlatPtr(&Processor::current()));
        if (--m_recursions == 0)
            m_lock.store(0, AK::memory_order_release);
        Processor::current().leave_critical(prev_flags);
    }

    [[nodiscard]] ALWAYS_INLINE bool is_locked() const
    {
        return m_lock.load(AK::memory_order_relaxed) != 0;
    }

    [[nodiscard]] ALWAYS_INLINE bool own_lock() const
    {
        return m_lock.load(AK::memory_order_relaxed) == FlatPtr(&Processor::current());
    }

    ALWAYS_INLINE void initialize()
    {
        m_lock.store(0, AK::memory_order_relaxed);
    }

private:
    Atomic<FlatPtr> m_lock { 0 };
    u32 m_recursions { 0 };
};

template<typename LockType>
class [[nodiscard]] ScopedSpinLock {

    AK_MAKE_NONCOPYABLE(ScopedSpinLock);

public:
    ScopedSpinLock() = delete;
    ScopedSpinLock& operator=(ScopedSpinLock&&) = delete;

    ScopedSpinLock(LockType& lock)
        : m_lock(&lock)
    {
        VERIFY(m_lock);
        m_prev_flags = m_lock->lock();
        m_have_lock = true;
    }

    ScopedSpinLock(ScopedSpinLock&& from)
        : m_lock(from.m_lock)
        , m_prev_flags(from.m_prev_flags)
        , m_have_lock(from.m_have_lock)
    {
        from.m_lock = nullptr;
        from.m_prev_flags = 0;
        from.m_have_lock = false;
    }

    ~ScopedSpinLock()
    {
        if (m_lock && m_have_lock) {
            m_lock->unlock(m_prev_flags);
        }
    }

    ALWAYS_INLINE void lock()
    {
        VERIFY(m_lock);
        VERIFY(!m_have_lock);
        m_prev_flags = m_lock->lock();
        m_have_lock = true;
    }

    ALWAYS_INLINE void unlock()
    {
        VERIFY(m_lock);
        VERIFY(m_have_lock);
        m_lock->unlock(m_prev_flags);
        m_prev_flags = 0;
        m_have_lock = false;
    }

    [[nodiscard]] ALWAYS_INLINE bool have_lock() const
    {
        return m_have_lock;
    }

private:
    LockType* m_lock { nullptr };
    u32 m_prev_flags { 0 };
    bool m_have_lock { false };
};

}