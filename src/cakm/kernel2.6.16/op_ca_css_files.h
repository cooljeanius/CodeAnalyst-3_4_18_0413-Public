#ifndef _OP_CA_CSS_FILES_H_
#define _OP_CA_CSS_FILES_H_

extern struct file_operations ca_css_depth_fops;
extern struct file_operations ca_css_tgid_fops;
extern struct file_operations ca_css_ppid_fops;
extern struct file_operations ca_css_invl_fops;
extern struct file_operations ca_css_bitness_fops;

extern ssize_t ca_css_depth_read(struct file *file,
					char __user *buf,
					size_t count,
					loff_t *offset);

extern ssize_t ca_css_depth_write(struct file *file,
					char const __user *buf,
					size_t count,
					loff_t *offset);

extern ssize_t ca_css_tgid_read(struct file * file,
					char __user * buf,
					size_t count,
					loff_t * offset);

extern ssize_t ca_css_tgid_write(struct file * file,
					char const __user * buf,
					size_t count,
					loff_t * offset);

extern ssize_t ca_css_bitness_read(struct file * file,
					char __user * buf,
					size_t count,
					loff_t * offset);

extern ssize_t ca_css_bitness_write(struct file * file,
					char const __user * buf,
					size_t count,
					loff_t * offset);

extern ssize_t ca_css_invl_read(struct file * file,
					char __user * buf,
					size_t count,
					loff_t * offset);

extern ssize_t ca_css_invl_write(struct file * file,
					char const __user * buf,
					size_t count,
					loff_t * offset);
#endif
