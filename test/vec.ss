(with (vec vlen vref vset t)
      (set! vec #vec)
      (set! vlen #veclen)
      (set! vref #vecref)
      (set! vset #vecset)
      (set! t (vec 4 5 6 7 8))
      ("display" t)
      ("display" (vlen t))
      ("display" (vref t 3))
      (vset t 3 1)
      ("display" t)
      )
