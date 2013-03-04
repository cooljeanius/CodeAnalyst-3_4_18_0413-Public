#if defined (CONFIG_CA_CSS)

#include <linux/fs.h>
#include "oprofile.h"
#include "op_ca_css_files.h"
#include "op_ca_css.h"

unsigned long ca_css_depth;
unsigned long ca_css_tgid;
unsigned long ca_css_bitness;
unsigned long ca_css_interval;

ssize_t ca_css_depth_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	return oprofilefs_ulong_to_user(ca_css_depth, buf, count,
					offset);
}


ssize_t ca_css_depth_write(struct file *file, char const __user *buf, size_t count, loff_t *offset)
{
	unsigned long val;
	int retval;

	if (*offset)
		return -EINVAL;

	retval = oprofilefs_ulong_from_user(&val, buf, count);
	if (retval)
		return retval;

	retval = ca_css_set_depth(val);

	if (retval)
		return retval;
	return count;
}


struct file_operations ca_css_depth_fops = {
	.read		= ca_css_depth_read,
	.write		= ca_css_depth_write
};


ssize_t ca_css_tgid_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
	return oprofilefs_ulong_to_user(ca_css_tgid, buf, count, offset);
}

ssize_t ca_css_tgid_write(struct file * file, char const __user * buf, size_t count, loff_t * offset)
{
	unsigned long val;
	int retval;

	if (*offset)
		return -EINVAL;

	retval = oprofilefs_ulong_from_user(&val, buf, count);
	if (retval)
		return retval;

	retval = ca_css_set_tgid(val);

	if (retval)
		return retval;
	return count;
}

struct file_operations ca_css_tgid_fops = {
	.read		= ca_css_tgid_read,
	.write		= ca_css_tgid_write
};

ssize_t ca_css_bitness_read(struct file * file, 
				char __user * buf, size_t count, loff_t * offset)
{
	return oprofilefs_ulong_to_user(ca_css_bitness, buf, count, offset);
}

ssize_t ca_css_bitness_write(struct file * file, 
			char const __user * buf, size_t count, loff_t * offset)
{
	unsigned long val;
	int retval;

	if (*offset)
		return -EINVAL;

	retval = oprofilefs_ulong_from_user(&val, buf, count);
	if (retval)
		return retval;

	retval = ca_css_set_bitness(val);

	if (retval)
		return retval;
	return count;
}

struct file_operations ca_css_bitness_fops = {
	.read		= ca_css_bitness_read,
	.write		= ca_css_bitness_write
};




ssize_t ca_css_invl_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
	return oprofilefs_ulong_to_user(ca_css_interval, buf, count, offset);
}

ssize_t ca_css_invl_write(struct file * file, char const __user * buf, size_t count, loff_t * offset)
{
	unsigned long val;
	int retval;

	if (*offset)
		return -EINVAL;

	retval = oprofilefs_ulong_from_user(&val, buf, count);
	if (retval)
		return retval;

	retval = ca_css_set_interval(val);

	if (retval)
		return retval;
	return count;
}

struct file_operations ca_css_invl_fops = {
	.read		= ca_css_invl_read,
	.write		= ca_css_invl_write
};

#endif // CONFIG_CA_CSS
