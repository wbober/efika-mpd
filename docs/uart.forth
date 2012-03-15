\ FORTH 

hex 
0xF0002C00 VALUE psc6_base 

\ fipbi (133Mhz / 32) in hex 
3EF148 VALUE psc6_fipbid32 

0x00 VALUE psc6_ctur_val 
0x24 VALUE psc6_ctlr_val 
0x0024 VALUE psc6_divider 

\ 115200 -> 0x1C200 
0x1C200 VALUE psc6_baudrate 

0 VALUE psc6_var 
0 VALUE psc6_var2 

: psc6_config 

\ Copy the baud rate if provided 
depth 
0<> IF 
   TO psc6_baudrate THEN 

\ correct bad baud rate 
psc6_baudrate 
0= IF 
   0x1C200 TO psc6_baudrate THEN 

psc6_fipbid32 psc6_baudrate / TO psc6_divider 

psc6_divider 0x00ff AND TO psc6_ctlr_val 
psc6_divider 8 >>a 0x00ff AND TO psc6_ctur_val 

hex 

0x0a psc6_base 0x08 + c! 

0x00000000 psc6_base 0x40 + l!      \ SICR set all to 0 
0xDD00   psc6_base 0x04 + w!      
psc6_ctur_val psc6_base 0x18 + c! 
psc6_ctlr_val psc6_base 0x1C + c! 

0x20 psc6_base 0x08 + c! 
0x30 psc6_base 0x08 + c! 
0x40 psc6_base 0x08 + c! 
0x50 psc6_base 0x08 + c! 
0x10 psc6_base 0x08 + c! 

0x00 psc6_base 0x10 + c! 
0x73 psc6_base 0x00 + c! \ MR1 
0x13 psc6_base 0x00 + c! \ MR2 

0x01 psc6_base 0x38 + c! 

0x0000 psc6_base 0x14 + w! 
0x00F8 psc6_base 0x6E + w! 
0x00F8 psc6_base 0x8E + w! 

0x001551124 0xf0000b00 l! 

0x05 psc6_base 0x08 + c! 
; 

: psc6_putchar 

depth 
0<> IF    
  psc6_base 0x0c + c! 
  1 ms 
  THEN 
; 

: psc6_write 
depth 
1 > IF 
  TO psc6_var 
  TO psc6_var2 
  psc6_var 0 DO 
    psc6_var2 I + c@ psc6_putchar 
    LOOP 
  THEN 
; 

psc6_config 

s" /builtin" find-device 
new-device 
   0xf0002c00 instance value regs 
   s" uart" device-name 
   s" serial" device-type 
   s" 2-wire Serial on PSC6" encode-string s" .description" property 
   0xf0002c00 0x100 reg 
   s" mpc5200-psc-uart" encode-string s" compatible" property 
   2 encode-int 4 encode-int encode+ 3 encode-int encode+ s" interrupts" property 
   \ 5 encode-int s" cell-index" property 
finish-device 