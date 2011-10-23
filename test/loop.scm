(with (foo)

      (set! foo
            (lambda (x)
              (if (#eq x 10000) 0
                  (foo (#add x 1)))))
      ("display" (foo 0)))
      
