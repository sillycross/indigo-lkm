#include "training_output.h"

#define MAX_OUTPUT_SIZE_BYTES (100 << 20)   // 100MB

const static char* x_training_output_name = "indigo_training_output";
static char* g_buffer = NULL;
static ssize_t g_length;
static struct proc_dir_entry* g_entry = NULL;
static DEFINE_SPINLOCK(g_spinlock);

// Reader is supposed to be called after there are no outstanding sockets running.
// Note that this is only true if the server has quited (the client having quited
// is not enough, since there may still be unsent data in kernel buffer)
//
static ssize_t __training_output_reader(struct file* file,
                                        char __user* ubuf,
                                        size_t count,
                                        loff_t* ppos)
{
    size_t len = READ_ONCE(g_length);
    ssize_t bytes_read;
    ssize_t max_bytes = len - *ppos;
    max_bytes = max_t(ssize_t, max_bytes, 0);
    max_bytes = min_t(ssize_t, max_bytes, (ssize_t)count);
    // copy_to_user returns the # of bytes that CANNOT be copied
    //
    bytes_read = max_bytes - copy_to_user(ubuf, g_buffer + *ppos, max_bytes);
    *ppos += bytes_read;
    return bytes_read;
}

// Clear the output file by writing a 'c' into the file
//
static ssize_t __training_output_clear(struct file *file,
                                       const char __user *ubuf,
                                       size_t count,
                                       loff_t *ppos)
{
    int max_bytes = min_t(size_t, count, 1);
    char buf[2];
    int bytes_copied = max_bytes - copy_from_user(buf, ubuf, max_bytes);
    if (bytes_copied < 1 || buf[0] != 'c')
    {
        TRACE_INFO("Unknown instruction to indigo training output. Ignored.");
        return -EINVAL;
    }
    indigo_training_output_get_lock();
    g_length = 0;
    indigo_training_output_unlock();
    TRACE_INFO("Indigo training output cleared.");
    *ppos += count;
    return count;
}

static struct file_operations g_proc_file_ops =
{
    .owner = THIS_MODULE,
    .read = __training_output_reader,
    .write = __training_output_clear
};

int WARN_UNUSED indigo_training_output_constructor(void)
{
#ifdef GEN_TRAINING_OUTPUTS
    g_buffer = vmalloc(MAX_OUTPUT_SIZE_BYTES);
    if (g_buffer == NULL)
    {
        TRACE_INFO("Failed to allocate buffer for training outputs: OOM");
        return 0;
    }
    g_length = 0;
    g_entry = proc_create(x_training_output_name, 0666, NULL, &g_proc_file_ops);
    if (g_entry == NULL)
    {
        TRACE_INFO("Failed to initialize output file at /proc/%s", x_training_output_name);
        return 0;
    }
    TRACE_INFO("Training outputs initialized successfully at /proc/%s", x_training_output_name);
#endif
    return 1;
}

void indigo_training_output_get_lock(void)
{
#ifdef GEN_TRAINING_OUTPUTS
    spin_lock(&g_spinlock);
#endif
}

void indigo_training_output_unlock(void)
{
#ifdef GEN_TRAINING_OUTPUTS
    spin_unlock(&g_spinlock);
#endif
}

void indigo_training_output_write(const char* format, ...)
{
#ifdef GEN_TRAINING_OUTPUTS
    va_list args;
    int full_length;
    Assert(g_buffer != NULL);
    va_start(args, format);
    full_length = vsnprintf(g_buffer + g_length, MAX_OUTPUT_SIZE_BYTES - g_length, format, args);
    va_end(args);
    g_length += full_length;
    // The full_length is the length if everything were written, excluding null space.
    // So we have overflowed if g_length >= the length of our buffer.
    //
    Assert(g_length < MAX_OUTPUT_SIZE_BYTES);
#endif
}

void indigo_training_output_destructor(void)
{
#ifdef GEN_TRAINING_OUTPUTS
    if (g_buffer != NULL)
    {
        vfree(g_buffer);
    }
    if (g_entry != NULL)
    {
        proc_remove(g_entry);
    }
#endif
}
