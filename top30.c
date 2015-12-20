#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eugene Gritskevich <qwaker.00@gmail.com>");
MODULE_DESCRIPTION("In-kernel top30");

#define TOP_COUNT 30
#define MAX_WORD_LENGTH 256

struct string {
    char* ptr;
    size_t size;
};

struct buffers {
    struct string read_buf;
};

static int string_compare(struct string* s1, struct string* s2) {
    size_t common = min(s1->size, s2->size);
    int res = strncmp(s1->ptr, s2->ptr, common);
    if (res < 0) {
        return -1;
    }
    if (res > 0) {
        return 1;
    }
    if (s1->size > common) {
        return 1;
    }
    if (s2->size > common) {
        return -1;
    }
    return 0;
}

static void string_copy(struct string* to, struct string* from) {
    memcpy(to, from, sizeof(struct string));
}

static void string_release(struct string* s) {
    kfree(s->ptr);
    s->ptr = NULL;
}

static struct string history[TOP_COUNT];
static int history_size = 0;
static struct mutex history_lock;

static ssize_t top30_new_read_buf(struct string* read_buf) {
    ssize_t result;
    size_t i;
    size_t offset = 0;
    char* buf;

    buf = kzalloc(history_size * MAX_WORD_LENGTH, GFP_KERNEL);
    if (unlikely(!buf)) {
        result = -ENOMEM;
        goto out;
    }

    if (mutex_lock_interruptible(&history_lock)) {
        result = -ERESTARTSYS;
        goto out_free;
    }

    for (i = 0; i < history_size; ++i) {
        memcpy(buf + offset, history[i].ptr, history[i].size);
        offset += history[i].size;
        buf[offset++] = '\n';
    }

    mutex_unlock(&history_lock);
    read_buf->ptr = buf;
    read_buf->size = offset;
    return 0;

 out_free:
    kfree(buf);

 out:
    return result;
}

static ssize_t top30_read(struct file *file,
                char __user * out,
                size_t size,
                loff_t * off)
{
    struct buffers* data = file->private_data;
    size_t read_size;
    ssize_t result;
    size_t to_read;

    if (data->read_buf.ptr == NULL) {
        ssize_t err = top30_new_read_buf(&data->read_buf);
        if (err) {
            result = err;
            goto out;
        }
    }

    read_size = data->read_buf.size;
    if (*off >= read_size) {
        result = 0;
    } else {
        to_read = min(read_size - (size_t)(*off), size);
        if (copy_to_user(out, data->read_buf.ptr + *off, to_read)) {
            result = -EFAULT;
            goto out;
        }
        result = to_read;
        *off += to_read;
    }

 out:
    return result;
}

static bool history_check_heap(void)
{
    size_t i;

    if (history_size > TOP_COUNT) {
        return false;
    }

    for (i = 1; i < history_size; ++i) {
        if (string_compare(history + i, history + (i - 1) / 2) > 0) {
            return false;
        }
    }

    return true;
}

static void history_pop_heap(void)
{
    size_t index = 0;
    size_t next_index;
    struct string* str;

    string_release(history + 0);

    --history_size;
    if (history_size == 0) {
        return;
    }

    str = history + history_size;
    while (index + index + 1 < history_size) {
        next_index = index + index + 1;
        if (next_index + 1 < history_size &&
                string_compare(history + next_index + 1, history + next_index) > 0)
        {
            ++next_index;
        }
        if (string_compare(str, history + next_index) >= 0) {
            break;
        }
        string_copy(history + index, history + next_index);
        index = next_index;
    }

    string_copy(history + index, str);

#ifdef DEBUG
    if (!history_check_heap()) {
        printk(KERN_INFO "Heap failed after pop!\n");
    }
#endif
}

static ssize_t history_push_heap(struct string* str)
{
    size_t index;
    size_t parent;

    if (history_size == TOP_COUNT) {
        if (string_compare(str, history + 0) >= 0) {
            return -1;
        }
        history_pop_heap();
    }

    index = history_size++;
    while (index > 0) {
        parent = (index - 1) / 2;
        if (string_compare(history + parent, str) >= 0) {
            break;
        }
        string_copy(history + index, history + parent);
        index = parent;
    }
    string_copy(history + index, str);

#ifdef DEBUG
    if (!history_check_heap()) {
        printk(KERN_INFO "Heap failed after push!\n");
    }
#endif

    return index;
}

static ssize_t top30_write(struct file *file,
                const char __user * in,
                size_t size,
                loff_t * off)
{
    ssize_t result;
    struct string buf_str;
    ssize_t index;

    if (size >= MAX_WORD_LENGTH) {
        buf_str.size = MAX_WORD_LENGTH - 1;
    } else {
        buf_str.size = size;
    }

    buf_str.ptr = kzalloc(buf_str.size, GFP_KERNEL);
    if (unlikely(!buf_str.ptr)) {
        result = -ENOMEM;
        goto out;
    }

    if (copy_from_user(buf_str.ptr, in, buf_str.size)) {
        result = -EFAULT;
        goto out_free;
    }
    result = size;

    if (mutex_lock_interruptible(&history_lock)) {
        result = -ERESTARTSYS;
        goto out_free;
    }
    index = history_push_heap(&buf_str);
    mutex_unlock(&history_lock);

    if (index >= 0) {
        buf_str.ptr = NULL;
    }

 out_free:
    string_release(&buf_str);

 out:
    return result;
}

static int top30_open(struct inode *inode, struct file *file)
{
    int err = 0;
    struct buffers *buf;
    buf = kzalloc(sizeof(*buf), GFP_KERNEL);
    if (unlikely(!buf)) {
        err = -ENOMEM;
        goto out;
    }
    file->private_data = buf;

 out:
    return err;
}

static int top30_release(struct inode *inode, struct file *file)
{
    struct buffers *buf = file->private_data;
    string_release(&buf->read_buf);
    kfree(buf);
    return 0;
}


static struct file_operations top30_fops = {
    .owner = THIS_MODULE,
    .open = top30_open,
    .read = top30_read,
    .write = top30_write,
    .release = top30_release,
    .llseek = noop_llseek
};

static struct miscdevice top30_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "top30",
    .fops = &top30_fops
};

static int __init top30_init(void)
{
    misc_register(&top30_misc_device);
    printk(KERN_INFO "top30 device has been registered\n");
    mutex_init(&history_lock);
    return 0;
}

static void __exit top30_exit(void)
{
    misc_deregister(&top30_misc_device);
    printk(KERN_INFO "top30 device has been unregistered\n");
}

module_init(top30_init);
module_exit(top30_exit);
