/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011 by Terraneo Federico             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/ 
 //Miosix kernel

#ifndef KERNEL_H
#define KERNEL_H

//Include settings.
#include "config/miosix_settings.h"
#include "interfaces/portability.h"
#include "kernel/scheduler/sched_types.h"
#include "kernel/syscalls_types.h"
#include <cstdlib>
#include <new>
#include <functional>

// some pthread functions are friends of Thread
#include <pthread.h>

/**
 * \namespace miosix
 * All user available kernel functions, classes are inside this namespace.
 */
namespace miosix {
    
/**
 * \addtogroup Kernel
 * \{
 */

/**
 * Disable interrupts, if interrupts were enable prior to calling this function.
 * 
 * Please note that starting from Miosix 1.51 disableInterrupts() and
 * enableInterrupts() can be nested. You can therefore call disableInterrupts()
 * multiple times as long as each call is matched by a call to
 * enableInterrupts().<br>
 * This replaced disable_and_save_interrupts() and restore_interrupts()
 *
 * disableInterrupts() cannot be called within an interrupt routine, but can be
 * called before the kernel is started (and does nothing in this case)
 */
void disableInterrupts();

/**
 * Enable interrupts.<br>
 * Please note that starting from Miosix 1.51 disableInterrupts() and
 * enableInterrupts() can be nested. You can therefore call disableInterrupts()
 * multiple times as long as each call is matched by a call to
 * enableInterrupts().<br>
 * This replaced disable_and_save_interrupts() and restore_interrupts()
 *
 * enableInterrupts() cannot be called within an interrupt routine, but can be
 * called before the kernel is started (and does nothing in this case)
 */
void enableInterrupts();

/**
 * Fast version of disableInterrupts().<br>
 * Despite faster, it has a couple of preconditions:
 * - calls to fastDisableInterrupts() can't be nested
 * - it can't be used in code that is called before the kernel is started
 */
inline void fastDisableInterrupts()
{
    miosix_private::doDisableInterrupts();
}

/**
 * Fast version of enableInterrupts().<br>
 * Despite faster, it has a couple of preconditions:
 * - calls to fastDisableInterrupts() can't be nested
 * - it can't be used in code that is called before the kernel is started,
 * because it will (incorreclty) lead to interrupts being enabled before the
 * kernel is started
 */
inline void fastEnableInterrupts()
{
    miosix_private::doEnableInterrupts();
}

/**
 * This class is a RAII lock for disabling interrupts. This call avoids
 * the error of not reenabling interrupts since it is done automatically.
 */
class InterruptDisableLock
{
public:
    /**
     * Constructor, disables interrupts.
     */
    InterruptDisableLock()
    {
        disableInterrupts();
    }

    /**
     * Destructor, reenables interrupts
     */
    ~InterruptDisableLock()
    {
        enableInterrupts();
    }

private:
    //Unwanted methods
    InterruptDisableLock(const InterruptDisableLock& l);
    InterruptDisableLock& operator= (const InterruptDisableLock& l);
};

/**
 * This class allows to temporarily re enable interrpts in a scope where
 * they are disabled with an InterruptDisableLock.<br>
 * Example:
 * \code
 *
 * //Interrupts enabled
 * {
 *     InterruptDisableLock dLock;
 *
 *     //Now interrupts disabled
 *
 *     {
 *         InterruptEnableLock eLock(dLock);
 *
 *         //Now interrupts back enabled
 *     }
 *
 *     //Now interrupts again disabled
 * }
 * //Finally interrupts enabled
 * \endcode
 */
class InterruptEnableLock
{
public:
    /**
     * Constructor, enables back interrupts.
     * \param l the InteruptDisableLock that disabled interrupts. Note that
     * this parameter is not used internally. It is only required to prevent
     * erroneous use of this class by making an instance of it without an
     * active InterruptEnabeLock
     */
    InterruptEnableLock(InterruptDisableLock& l)
    {
        enableInterrupts();
    }

    /**
     * Destructor.
     * Disable back interrupts.
     */
    ~InterruptEnableLock()
    {
        disableInterrupts();
    }

private:
    //Unwanted methods
    InterruptEnableLock(const InterruptEnableLock& l);
    InterruptEnableLock& operator= (const InterruptEnableLock& l);
};

/**
 * This class is a RAII lock for disabling interrupts. This call avoids
 * the error of not reenabling interrupts since it is done automatically.
 * As opposed to InterruptDisableLock, this version doesn't support nesting
 */
class FastInterruptDisableLock
{
public:
    /**
     * Constructor, disables interrupts.
     */
    FastInterruptDisableLock()
    {
        fastDisableInterrupts();
    }

    /**
     * Destructor, reenables interrupts
     */
    ~FastInterruptDisableLock()
    {
        fastEnableInterrupts();
    }

private:
    //Unwanted methods
    FastInterruptDisableLock(const FastInterruptDisableLock& l);
    FastInterruptDisableLock& operator= (const FastInterruptDisableLock& l);
};

/**
 * This class allows to temporarily re enable interrpts in a scope where
 * they are disabled with an FastInterruptDisableLock.
 */
class FastInterruptEnableLock
{
public:
    /**
     * Constructor, enables back interrupts.
     * \param l the InteruptDisableLock that disabled interrupts. Note that
     * this parameter is not used internally. It is only required to prevent
     * erroneous use of this class by making an instance of it without an
     * active InterruptEnabeLock
     */
    FastInterruptEnableLock(FastInterruptDisableLock& l)
    {
        fastEnableInterrupts();
    }

    /**
     * Destructor.
     * Disable back interrupts.
     */
    ~FastInterruptEnableLock()
    {
        fastDisableInterrupts();
    }

private:
    //Unwanted methods
    FastInterruptEnableLock(const FastInterruptEnableLock& l);
    FastInterruptEnableLock& operator= (const FastInterruptEnableLock& l);
};

/**
 * Pause the kernel.<br>Interrupts will continue to occur, but no preemption is
 * possible. Call to this function are cumulative: if you call pauseKernel()
 * two times, you need to call restartKernel() two times.<br>Pausing the kernel
 * must be avoided if possible because it is easy to cause deadlock. Calling
 * file related functions (fopen, Directory::open() ...), serial port related
 * functions (printf ...) or kernel functions that cannot be called when the
 * kernel is paused will cause deadlock. Therefore, if possible, it is better to
 * use a Mutex instead of pausing the kernel<br>This function is safe to be
 * called even before the kernel is started. In this case it has no effect.
*/
void pauseKernel();

/**
 * Restart the kernel.<br>This function will yield immediately if a tick has
 * been missed. Since calls to pauseKernel() are cumulative, if you call
 * pauseKernel() two times, you need to call restartKernel() two times.<br>
 * This function is safe to be called even before the kernel is started. In this
 * case it has no effect.
 */
void restartKernel();

/**
 * \return true if interrupts are enabled
 */
bool areInterruptsEnabled();

/**
 * This class is a RAII lock for pausing the kernel. This call avoids
 * the error of not restarting the kernel since it is done automatically.
 */
class PauseKernelLock
{
public:
    /**
     * Constructor, pauses the kernel.
     */
    PauseKernelLock()
    {
        pauseKernel();
    }

    /**
     * Destructor, restarts the kernel
     */
    ~PauseKernelLock()
    {
        restartKernel();
    }

private:
    //Unwanted methods
    PauseKernelLock(const PauseKernelLock& l);
    PauseKernelLock& operator= (const PauseKernelLock& l);
};

/**
 * This class allows to temporarily restart kernel in a scope where it is
 * paused with an InterruptDisableLock.<br>
 * Example:
 * \code
 *
 * //Kernel started
 * {
 *     PauseKernelLock dLock;
 *
 *     //Now kernel paused
 *
 *     {
 *         RestartKernelLock eLock(dLock);
 *
 *         //Now kernel back started
 *     }
 *
 *     //Now kernel again paused
 * }
 * //Finally kernel started
 * \endcode
 */
class RestartKernelLock
{
public:
    /**
     * Constructor, restarts kernel.
     * \param l the PauseKernelLock that disabled interrupts. Note that
     * this parameter is not used internally. It is only required to prevent
     * erroneous use of this class by making an instance of it without an
     * active PauseKernelLock
     */
    RestartKernelLock(PauseKernelLock& l)
    {
        restartKernel();
    }

    /**
     * Destructor.
     * Disable back interrupts.
     */
    ~RestartKernelLock()
    {
        pauseKernel();
    }

private:
    //Unwanted methods
    RestartKernelLock(const RestartKernelLock& l);
    RestartKernelLock& operator= (const RestartKernelLock& l);
};

/**
 * \internal
 * Start the kernel.<br>You must create at least one thread using Thread::create
 * before starting the kernel. There is no way to stop the kernel once it is
 * started, except a (software or hardware) system reset.<br>
 * Calls errorHandler(OUT_OF_MEMORY) if there is no heap to create the idle
 * thread. If the function succeds in starting the kernel, it never returns;
 * otherwise it will call errorHandler(OUT_OF_MEMORY) and then return
 * immediately. startKernel() must not be called when the kernel is already
 * started.
 */
void startKernel();

/**
 * Return true if kernel is running, false if it is not started, or paused.<br>
 * Warning: disabling/enabling interrupts does not affect the result returned by
 * this function.
 * \return true if kernel is running (started && not paused)
 */
bool isKernelRunning();

/**
 * Returns the current kernel tick.<br>Can be called also with interrupts
 * disabled and/or kernel paused.
 * \return current kernel tick
 */
long long getTick();

//Declaration of the struct, definition follows below
struct SleepData;

//Forwrd declaration of classes
class MemoryProfiling;
class Mutex;
class ConditionVariable;

/**
 * This class represents a thread. It has methods for creating, deleting and
 * handling threads.<br>It has private constructor and destructor, since memory
 * for a thread is handled by the kernel.<br>To create a thread use the static
 * producer method create().<br>
 * Methods that have an effect on the current thread, that is, the thread that
 * is calling the method are static.<br>
 * Calls to non static methods must be done with care, because a thread can
 * terminate at any time. For example, if you call wakeup() on a terminated
 * thread, the behavior is undefined.
 */
class Thread
{
public:

    /**
     * Thread options, can be passed to Thread::create to set additional options
     * of the thread.
     * More options can be specified simultaneously by ORing them together.
     * The DEFAULT option indicates the default thread creation.
     */
    enum Options
    {
        DEFAULT=0,    ///< Default thread options
        JOINABLE=1<<0 ///< Thread is joinable instead of detached
    };

    /**
     * Producer method, creates a new thread.
     * \param startfunc the entry point function for the thread
     * \param stacksize size of thread stack, its minimum is the constant
     * STACK_MIN.
     * The size of the stack must be divisible by 4, otherwise it will be
     * rounded to a number divisible by 4.
     * \param priority the thread's priority, between 0 (lower) and
     * PRIORITY_MAX-1 (higher)
     * \param argv a void* pointer that is passed as pararmeter to the entry
     * point function
     * \param options thread options, such ad Thread::JOINABLE
     * \return a reference to the thread created, that can be used, for example,
     * to delete it, or NULL in case of errors.
     *
     * Calls errorHandler(INVALID_PARAMETERS) if stacksize or priority are
     * invalid, and errorHandler(OUT_OF_MEMORY) if the heap is full.
     * Can be called when the kernel is paused.
     * Note: this is the only method of this class that can be called BEFORE the
     * kernel is started.
     */
    static Thread *create(void *(*startfunc)(void *), unsigned int stacksize,
                            Priority priority=Priority(), void *argv=NULL,
                            unsigned short options=DEFAULT);

    /**
     * Same as create(void (*startfunc)(void *), unsigned int stacksize,
     * Priority priority=1, void *argv=NULL)
     * but in this case the entry point of the thread returns a void*
     * \param startfunc the entry point function for the thread
     * \param stacksize size of thread stack, its minimum is the constant
     * STACK_MIN.
     * The size of the stack must be divisible by 4, otherwise it will be
     * rounded to a number divisible by 4.
     * \param priority the thread's priority, between 0 (lower) and
     * PRIORITY_MAX-1 (higher)
     * \param argv a void* pointer that is passed as pararmeter to the entry
     * point function
     * \param options thread options, such ad Thread::JOINABLE
     * \return a reference to the thread created, that can be used, for example,
     * to delete it, or NULL in case of errors.
     */
    static Thread *create(void (*startfunc)(void *), unsigned int stacksize,
                            Priority priority=Priority(), void *argv=NULL,
                            unsigned short options=DEFAULT);

    /**
     * When called, suggests the kernel to pause the current thread, and run
     * another one.
     * <br>CANNOT be called when the kernel is paused.
     */
    static void yield();

    /**
     * This method needs to be called periodically inside the thread's main
     * loop.
     * \return true if somebody outside the thread called terminate() on this
     * thread.
     *
     * If it returns true the thread must free all resources and terminate by
     * returning from its main function.
     * <br>Can be called when the kernel is paused.
     */
    static bool testTerminate();

    /**
     * Put the thread to sleep for a number of milliseconds.<br>The actual
     * precision depends on the kernel tick used. If the specified wait time is
     * lower than the tick accuracy, the thread will be put to sleep for one
     * tick.<br>Maximum sleep time is (2^32-1) / TICK_FREQ. If a sleep time
     * higher than that value is specified, the behaviour is undefined.
     * \param ms the number of millisecond. If it is ==0 this method will
     * return immediately
     *
     * CANNOT be called when the kernel is paused.
     */
    static void sleep(unsigned int ms);
    
    /**
     * Put the thread to sleep until the specified absolute time is reached.
     * If the time is in the past, returns immediately.
     * To make a periodic thread, this is the recomended way
     * \code
     * void periodicThread()
     * {
     *     //Run every 90 milliseconds
     *     const int period=static_cast<int>(TICK_FREQ*0.09);
     *     long long tick=getTick();
     *     for(;;)
     *     {
     *         //Do work
     *         tick+=period;
     *         Thread::sleepUntil(tick);
     *     }
     * }
     * \endcode
     * \param absoluteTime when to wake up
     *
     * CANNOT be called when the kernel is paused.
     */
    static void sleepUntil(long long absoluteTime);

    /**
     * Return a pointer to the Thread class of the current thread.
     * \return a pointer to the current thread.
     *
     * Can be called when the kernel is paused.
     */
    static Thread *getCurrentThread();

    /**
     * Check if a thread exists
     * \param p thread to check
     * \return true if thread exists, false if does not exist or has been
     * deleted. A joinable thread is considered existing until it has been
     * joined, even if it returns from its entry point (unless it is detached
     * and terminates).
     *
     * Can be called when the kernel is paused.
     */
    static bool exists(Thread *p);

    /**
     * Returns the priority of a thread.<br>
     * To get the priority of the current thread use:
     * \code Thread::getCurrentThread()->getPriority(); \endcode
     * If the thread is currently locking one or more mutexes, this member
     * function returns the current priority, which can be higher than the
     * original priority due to priority inheritance.
     * \return current priority of the thread
     *
     * Can be called when the kernel is paused.
     */
    Priority getPriority();

    /**
     * Set the priority of this thread.<br>
     * This member function changed from previous Miosix versions since it is
     * now static. This implies a thread can no longer set the priority of
     * another thread.
     * \param pr desired priority. Must be 0<=pr<PRIORITY_MAX
     *
     * Calls errorHandler(INVALID_PARAMETERS) if pr is not within bounds.
     *
     * Can be called when the kernel is paused.
     */
    static void setPriority(Priority pr);

    /**
     * Suggests a thread to terminate itself. Note that this method only makes
     * testTerminate() return true on the specified thread. If the thread does
     * not call testTerminate(), or if it calls it but does not delete itself
     * by returning from entry point function, it will NEVER
     * terminate. The user is responsible for implementing correctly this
     * functionality.<br>Thread termination is implemented like this to give
     * time to a thread to deallocate resources, close files... before
     * terminating. <br>Can be called when the kernel is paused.
     */
    void terminate();

    /**
     * This method stops the thread until another thread calls wakeup() on this
     * thread.<br>Calls to wait are not cumulative. If wait() is called two
     * times, only one call to wakeup() is needed to wake the thread.
     * <br>CANNOT be called when the kernel is paused.
     */
    static void wait();

    /**
     * Wakeup a thread.
     * <br>CANNOT be called when the kernel is paused.
     */
    void wakeup();

    /**
     * Wakeup a thread.
     * <br>Can be called when the kernel is paused.
     */
    void PKwakeup();

    /**
     * Detach the thread if it was joinable, otherwise do nothing.<br>
     * If called on a deleted joinable thread on which join was not yet called,
     * it allows the thread's memory to be deallocated.<br>
     * If called on a thread that is not yet deleted, the call detaches the
     * thread without deleting it.
     * If called on an already detached thread, it has undefined behaviour.
     */
    void detach();

    /**
     * \return true if the thread is detached
     */
    bool isDetached() const;

    /**
     * Wait until a joinable thread is terminated.<br>
     * If the thread already terminated, this function returns immediately.<br>
     * Calling join() on the same thread multiple times, from the same or
     * multiple threads is not recomended, but in the current implementation
     * the first call will wait for join, and the other will return false.<br>
     * Trying to join the thread join is called in returns false, but must be
     * avoided.<br>
     * Calling join on a detached thread might cause undefined behaviour.
     * \param result If the entry point function of the thread to join returns
     * void *, the return value of the entry point is stored here, otherwise
     * the content of this variable is undefined. If NULL is passed as result
     * the return value will not be stored.
     * \return true on success, false on failure
     */
    bool join(void** result=NULL);

    /**
     * Same as get_current_thread(), but meant to be used insida an IRQ, when
     *interrupts are disabled or when the kernel is paused.
     */
    static Thread *IRQgetCurrentThread();

    /**
     * Same as getPriority(), but meant to be used inside an IRQ, when
     * interrupts are disabled or when the kernel is paused.
     */
    Priority IRQgetPriority();

    /**
     * Same as wait(), but is meant to be used only inside an IRQ or when
     * interrupts are disabled.<br>
     * Note: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled; it returns immediately, so
     * the user is responsible for re-enabling interrupts and calling yield to
     * effectively put the thread in wait status.
     *
     * \code
     * disableInterrupts();
     * ...
     * Thread::IRQwait();//Return immediately
     * enableInterrupts();
     * Thread::yield();//After this, thread is in wait status
     * \endcode
     */
    static void IRQwait();

    /**
     * Same as wakeup(), but is meant to be used only inside an IRQ or when
     * interrupts are disabled.
     */
    void IRQwakeup();

    /**
     * Same as exists() but is meant to be called only inside an IRQ or when
     * interrupts are disabled.
     */
    static bool IRQexists(Thread *p);

    /**
     * \internal
     * This method is only meant to implement functions to check the available
     * stack in a thread. Returned pointer is constant because modifying the
     * stack through it must be avoided.
     * \return pointer to bottom of stack of current thread.
     */
    static const unsigned int *getStackBottom();

    /**
     * \internal
     * \return the size of the stack of the current thread.
     */
    static const int getStackSize();
	
private:
    //Unwanted methods
    Thread(const Thread& p);///< No public copy constructor
    Thread& operator = (const Thread& p);///< No publc operator =

    class ThreadFlags
    {
    public:
        /**
         * Constructor, sets flags to default.
         */
        ThreadFlags(): flags(0) {}

        /**
         * Set the wait flag of the thread.
         * Can only be called with interrupts enabled or within an interrupt.
         * \param waiting if true the flag will be set, otherwise cleared
         */
        void IRQsetWait(bool waiting);

        /**
         * Set the wait_join flag of the thread.
         * Can only be called with interrupts enabled or within an interrupt.
         * \param waiting if true the flag will be set, otherwise cleared
         */
        void IRQsetJoinWait(bool waiting);

        /**
         * Set wait_cond flag of the thread.
         * Can only be called with interrupts enabled or within an interrupt.
         * \param waiting if true the flag will be set, otherwise cleared
         */
        void IRQsetCondWait(bool waiting);

        /**
         * Set the sleep flag of the thread.
         * Can only be called with interrupts enabled or within an interrupt.
         * \param sleeping if true the flag will be set, otherwise cleared
         */
        void IRQsetSleep(bool sleeping);

        /**
         * Set the deleted flag of the thread. This flag can't be cleared.
         * Can only be called with interrupts enabled or within an interrupt.
         */
        void IRQsetDeleted();

        /**
         * Set the sleep flag of the thread. This flag can't be cleared.
         * Can only be called with interrupts enabled or within an interrupt.
         */
        void IRQsetDeleting()
        {
            flags |= DELETING;
        }

        /**
         * Set the detached flag. This flag can't be cleared.
         * Can only be called with interrupts enabled or within an interrupt.
         */
        void IRQsetDetached()
        {
            flags |= DETACHED;
        }

        /**
         * \return true if the wait flag is set
         */
        bool isWaiting() const { return flags & WAIT; }

        /**
         * \return true if the sleep flag is set
         */
        bool isSleeping() const { return flags & SLEEP; }

        /**
         * \return true if the deleted and the detached flags are set
         */
        bool isDeleted() const { return (flags & 0x14)==0x14; }

        /**
         * \return true if the thread has been deleted, but its resources cannot
         * be reclaimed because it has not yet been joined
         */
        bool isDeletedJoin() const { return flags & DELETED; }

        /**
         * \return true if the deleting flag is set
         */
        bool isDeleting() const { return flags & DELETING; }

        /**
         * \return true if the thread is in the ready status
         */
        bool isReady() const { return (flags & 0x67)==0; }

        /**
         * \return true if the thread is detached
         */
        bool isDetached() const { return flags & DETACHED; }

        /**
         * \return true if the thread is waiting a join
         */
        bool isWaitingJoin() const { return flags & WAIT_JOIN; }

        /**
         * \return true if the thread is waiting on a condition variable
         */
        bool isWaitingCond() const { return flags & WAIT_COND; }

    private:
        ///\internal Thread is in the wait status. A call to wakeup will change
        ///this
        static const unsigned int WAIT=1<<0;

        ///\internal Thread is sleeping.
        static const unsigned int SLEEP=1<<1;

        ///\internal Thread is deleted. It will continue to exist until the
        ///idle thread deallocates its resources
        static const unsigned int DELETED=1<<2;

        ///\internal Somebody outside the thread asked this thread to delete
        ///itself.<br>This will make Thread::testTerminate() return true.
        static const unsigned int DELETING=1<<3;

        ///\internal Thread is detached
        static const unsigned int DETACHED=1<<4;

        ///\internal Thread is waiting for a join
        static const unsigned int WAIT_JOIN=1<<5;

        ///\internal Thread is waiting on a condition variable
        static const unsigned int WAIT_COND=1<<6;

        unsigned short flags;///<\internal flags are stored here
    };

    /**
     * Constructor, initializes thread data.
     * \param priority thread's priority
     * \param watermark pointer to watermark area
     * \param stacksize thread's stack size
     */
    Thread(unsigned int *watermark, unsigned int stacksize):
        schedData(), flags(), savedPriority(0), mutexLocked(0), mutexWaiting(0),
        watermark(watermark), ctxsave(), stacksize(stacksize)
    {
        joinData.waitingForJoin=NULL;
    }

    /**
     * Destructor, does nothing.
     */
    ~Thread() {}

    /**
     * Thread launcher, all threads start from this member function, which calls
     * the user specified entry point. When the entry point function returns,
     * it marks the thread as deleted so that the idle thread can dellocate it.
     * If exception handling is enebled, this member function also catches any
     * exception that propagates through the entry point.
     * \param threadfunc pointer to the entry point function
     * \param argv argument passed to the entry point
     */
    static void threadLauncher(void *(*threadfunc)(void*), void *argv);

    //Thread data
    SchedulerData schedData; ///< Scheduler data, only used by class Scheduler
    ThreadFlags flags;///< thread status
    ///Saved priority. Its value is relevant only if mutexLockedCount>0; it
    ///stores the value of priority that this thread will have when it unlocks
    ///all mutexes. This is because when a thread locks a mutex its priority
    ///can change due to priority inheritance.
    Priority savedPriority;
    ///List of mutextes locked by this thread
    Mutex *mutexLocked;
    ///If the thread is waiting on a Mutex, mutexWaiting points to that Mutex
    Mutex *mutexWaiting;
    unsigned int *watermark;///< pointer to watermark area
    unsigned int ctxsave[CTXSAVE_SIZE];///< Holds cpu registers during ctxswitch
    unsigned int stacksize;///< Contains stack size
    ///This union is used to join threads. When the thread to join has not yet
    ///terminated and no other thread called join it contains (Thread *)NULL,
    ///when a thread calls join on this thread it contains the thread waiting
    ///for the join, and when the thread terminated it contains (void *)result
    union
    {
        Thread *waitingForJoin;///<Thread waiting to join this
        void *result;          ///<Result returned by entry point
    } joinData;
    /// Per-thread instance of data to make C++ exception handling thread safe.
    ExceptionHandlingData exData;
    
    //friend functions
    //Needs access to watermark, ctxsave
    friend void miosix_private::IRQstackOverflowCheck();
    //Need access to status
    friend void IRQaddToSleepingList(SleepData *x);
    //Needs access to status
    friend bool IRQwakeThreads();
    //Needs access to watermark, status, next
    friend void *idleThread(void *argv);
    //Needs to create the idle thread
    friend void startKernel();
    //Needs threadLauncher
    friend void miosix_private::initCtxsave(unsigned int *ctxsave,
            void *(*pc)(void *), unsigned int *sp, void *argv);
    //Needs access to priority, savedPriority, mutexLocked and flags.
    friend class Mutex;
    //Needs access to flags
    friend class ConditionVariable;
    //Needs access to flags, schedData
    friend class PriorityScheduler;
    //Needs access to flags, schedData
    friend class ControlScheduler;
    //Needs access to flags, schedData
    friend class EDFScheduler;
    //Needs access to flags
    friend int ::pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
    //Needs access to flags
    friend int ::pthread_cond_signal(pthread_cond_t *cond);
    //Needs access to flags
    friend int ::pthread_cond_broadcast(pthread_cond_t *cond);
    //Needs access to exData
    friend class ExceptionHandlingAccessor;
};

/**
 * Function object to compare the priority of two threads.
 */
class LowerPriority: public std::binary_function<Thread*,Thread*,bool>
{
public:
    /**
     * \param a first thread to compare
     * \param b second thread to compare
     * \return true if a->getPriority() < b->getPriority()
     *
     * Can be called when the kernel is paused. or with interrupts disabled
     */
    bool operator() (Thread* a, Thread *b)
    {
        return a->getPriority() < b->getPriority();
    }
};

/**
 * \internal
 * \struct Sleep_data
 * This struct is used to make a list of sleeping threads.
 * It is used by the kernel, and should not be used by end users.
 */
struct SleepData
{
    ///\internal Thread that is sleeping
    Thread *p;
    
    ///\internal When this number becomes equal to the kernel tick,
    ///the thread will wake
    long long wakeup_time;
    
    SleepData *next;///<\internal Next thread in the list
};

/**
 * \}
 */

}; //namespace miosix

#endif //KERNEL_H
