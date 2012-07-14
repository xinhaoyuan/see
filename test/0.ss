("display" (with (callcc) (set! callcc (lambda (x) (call/cc x))) ((callcc callcc) (callcc callcc))))
