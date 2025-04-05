.text
.global longest_increasing_subsequence

longest_increasing_subsequence:
  mov x8, 0          
  mov x11, 1         

loop_outer:
  cmp x8, x2        
  bge loop_outer_end 

  str x11, [x1, x8, lsl 3] 

  mov x9, 0          
loop_inner:
  cmp x9, x8         
  bge loop_inner_end 

  ldr x12, [x0, x8, lsl 3] 
  ldr x13, [x0, x9, lsl 3] 
  cmp x13, x12       
  bge skip_iteration  

  ldr x14, [x1, x8, lsl 3] 
  ldr x15, [x1, x9, lsl 3] 
  add x15, x15, 1     

  cmp x14, x15       
  bge skip_iteration  

  str x15, [x1, x8, lsl 3] 

skip_iteration:
  add x9, x9, 1      
  b loop_inner        

loop_inner_end:
  add x8, x8, 1      
  b loop_outer        

loop_outer_end:
  mov x8, 0          
  mov x0, 0          

find_max:
  cmp x8, x2        
  bge end_of_function 

  ldr x9, [x1, x8, lsl 3] 
  cmp x0, x9        
  bge continue_max   

  mov x0, x9        

continue_max:
  add x8, x8, 1     
  b find_max        

end_of_function:
  ret
