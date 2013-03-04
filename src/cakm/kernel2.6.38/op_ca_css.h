#ifndef _OP_CA_CSS_H_
#define _OP_CA_CSS_H_

extern unsigned long ca_css_depth;
extern unsigned long ca_css_tgid;
extern unsigned long ca_css_bitness;
extern unsigned long ca_css_interval;

extern int ca_css_set_depth(unsigned long depth);
extern int ca_css_set_tgid(unsigned long tgid);
extern int ca_css_set_bitness(unsigned long bitness);
extern int ca_css_set_interval(unsigned long interval);

#endif
