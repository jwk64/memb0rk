#lang racket

; Note: this works:
; (eval (read (open-input-string  "(+ 1 2) trailing garbage")))
;
; Also, can do:
;
; (define ns (make-base-namespace))
; (eval '(define x 3) ns)
; (eval 'x ns)
;
; If namespace unspecified, i.e.:
; (eval '(define x 3))
; Then x is defined in local scope, i.e. the following would then return 3:
; x

; See end of https://docs.racket-lang.org/guide/eval.html:
(define-namespace-anchor a)
(define ns (namespace-anchor->namespace a))

(define (parse-port in out)
  (define c (read-char in))
  (if
   (equal? c eof)
   (void)
   (begin
     (display
      (if
       (equal? c #\$)
       (~a (eval (read in) ns))
       (string c))
      out)
     (parse-port in out))
   ))

(define (parse-string in)
  (define out (open-output-string))
  (parse-port (open-input-string in) out)
  (get-output-string out))

(parse-port (current-input-port) (current-output-port))
