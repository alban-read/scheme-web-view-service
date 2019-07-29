 

(define escape-pressed?
  (lambda () ((foreign-procedure "EscapeKeyPressed" () ptr))))

(define gc
  (lambda () (collect) (collect) (collect) (collect)))

(define startserver
  (lambda (n f)
    ((foreign-procedure "start_web_server" (int string) ptr)
      n
      f)))

(define homepage
  (lambda (url)
    ((foreign-procedure "scheme_home_page" (string) ptr) url)))

(define navigate
  (lambda (url)
    ((foreign-procedure "navigate" (string) ptr) url)))

(define sleep
 (lambda (ms)
    ((foreign-procedure "scheme_wait" (int) ptr) ms)))

(define yield
 (lambda (ms)
    ((foreign-procedure "scheme_yield" (int) ptr) ms)))



(define web-message
  (lambda (msg)
    ((foreign-procedure "scheme_post_message" (string) ptr) msg )))

(define web-value
  (lambda (script vname)
    ((foreign-procedure "web_value" (string string) ptr) script vname)))


(define web-capture
  (lambda (msg)
    ((foreign-procedure "scheme_capture_screen" (string) ptr) msg )))

(define web-uri
  (lambda ()
    ((foreign-procedure "scheme_get_source" () ptr)  )))


(define web-eval 
  (lambda (script)
    (web-message (string-append "::api:8:" script))))


(define web-exec
  (lambda (script fname)
    (web-message (string-append "::api:9:" fname "::" script))))

 (define web-load-document
  (lambda (fname) 
        ((foreign-procedure "web_load_document" (string ) ptr) fname )))


(define-syntax dotimes
  (syntax-rules ()
    [(_ n body ...)
     (let loop ([i n])  
       (when (and (< 0 i) (not (escape-pressed?)))
         body
         ...
         (loop (- i 1))))]))

(define-syntax while
  (syntax-rules ()
    [(while test body ...)
    (let loop ()
       (if (and (not (escape-pressed?)) test)
           (begin body ... (loop))))]))

(define-syntax for
  (syntax-rules (for in to step)
    [(for i in elements body ...)
     (for-each (lambda (i) body ...) elements)]
    [(for i from start to end step is body ...)
     (let ([condition (lambda (i)
                        (cond
                          [(< is 0) (< i end)]
                          [(> is 0) (> i end)]
                          [else #f]))])
       (do ([i start (+ i is)]) ((condition i) (void)) body ...))]
    [(for i from start to end body ...)
     (do ([i start (+ i 1)]) ((> i end) (void)) body ...)]))

(define-syntax try
  (syntax-rules (catch)
    [(_ body (catch catcher))
     (call-with-current-continuation
       (lambda (exit)
         (with-exception-handler
           (lambda (condition) (catcher condition) (exit condition))
           (lambda () body))))]))

 
;; call backs are grim; and fiendish; best avoided.
(define register-callback
  (lambda (n f)
    ((foreign-procedure "register_callback"
       (int unsigned-64)
       ptr)
      n
      f)))

(define register-api
  (lambda (n f)
    ((foreign-procedure "register_api_callback"
       (int unsigned-64)
       ptr)
      n
      f)))

(define expressions "")

(define clrexp (lambda () (set! expressions '())))

(define transcript '())

(define clrt (lambda () (set! transcript '())))

(define tprintln
  (lambda (x) (set! transcript (cons x transcript))))

(define transcript0
  (lambda (x) 
    (web-message (string-append "::api:7:" x))
    (set! transcript (cons x transcript))))

(define pretty-print-port pretty-print)

(define pretty-print-string
  (lambda (x)
    (with-output-to-string (lambda () (pretty-print-port x)))))

(define pretty-print-transcript
  (lambda (x)
    (transcript0
      (with-output-to-string (lambda () (pretty-print-port x))))))

(define display-port display)

(define newline-port newline)

(define display-transcript
  (lambda (x)
    (transcript0
      (with-output-to-string (lambda () (display-port x))))))

(define newline-transcript
  (lambda () (display-transcript #\newline)))

(define display
  (case-lambda
    [(x p)
     (unless (and (output-port? p) (textual-port? p))
       (raise "not a text port"))
     (display-port x p)]
    [(x) (display-transcript x)]))

(define display-string (lambda (x) (display x)))

(define newline
  (case-lambda
    [(p)
     (unless (and (output-port? p) (textual-port? p))
       (raise "not a text port"))
     (newline-port p)]
    [() (newline-transcript)]))

(define blank
  (lambda ()
    ((foreign-procedure "clear_transcript" () void))))

(define (println . args)
  (apply transcript0 args)
  (transcript0 "\r\n"))


(define web-result 
 (lambda (result)
  (display "web result:")
  (display result)
  (display "\n") ))  



;; control return; shift return calls this
(define eval->string
  (lambda (x)
    (define os (open-output-string))
    (try (begin
          (set! expressions x)
           (let* ([is (open-input-string x)])
             (let ([expr (read is)])
               (while
                 (not (eq? #!eof expr))
                 (try (begin (write (eval expr) os) (gc))
                      (catch
                        (lambda (c)
                          (println
                            (call-with-string-output-port
                              (lambda (p) (display-condition c p)))))))
                 (newline os)
                 (set! expr (read is)))))
           (get-output-string os))
         (catch
           (lambda (c)
             (println
               (call-with-string-output-port
                 (lambda (p) (display-condition c p)))))))))

;; eval and post back result.
(define eval->string-post-back 
 (lambda (x) 
 (try
   (web-message (string-append "::eval_reply:" (eval->string x)))
 (catch (lambda(x)  (web-message "::snafu:"))))))



;; strapped to alt F  
(define eval->pretty
  (lambda (x)
    (define os (open-output-string))
    (try (begin
           (let* ([is (open-input-string x)])
             (let ([expr (read is)])
               (while
                 (not (eq? #!eof expr))
                 (try (begin (pretty-print-port expr os))
                      (catch
                        (lambda (c)
                          (println
                            (call-with-string-output-port
                              (lambda (p) (display-condition c p)))))))
                 (newline os)
                 (set! expr (read is)))))
           (get-output-string os))
         (catch
           (lambda (c)
             (println
               (call-with-string-output-port
                 (lambda (p) (display-condition c p)))))))))

(define eval->pretty-post-back 
 (lambda (x) 
   (web-message (string-append "::api_reply:5:" (eval->pretty x)))))
 
(define save-evaluator-text
  (lambda (x) (set! expressions x)))

(define echo-test
  (lambda (x) (list->string (reverse (string->list x)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; api calls from browser window. 

(define api-call-undefined
  (lambda (n s1 )
    (string-append
      "!ERR:api call #"
      (number->string n)
      " is undefined.")))

(define api-call-echo
  (lambda (n s1)
    (string-append (number->string n) ":" s1 )))


(define api-calls (make-vector 64))
(define api-call-limit (vector-length api-calls))
(vector-fill! api-calls api-call-undefined)

(define api-call
  (lambda (n s1 )
    (if (< n api-call-limit)
        (let ([f (vector-ref api-calls n)])
          (if (procedure? f) (apply f (list n s1 )))))))


(define api-call-get-transcript
  (lambda (n s1 )
    (apply string-append 
      (cons ";;; transcript\n" (reverse transcript)))))


(define message-response-get-transcript 
 (lambda ()
   (apply string-append 
      (cons "::transcript_reply:" (reverse transcript)))))


(vector-set! api-calls 1 api-call-get-transcript)

(define api-call-clr-transcript
  (lambda (n s1 ) (clrt) "Ok"))

(vector-set! api-calls 2 api-call-clr-transcript)

(define api-call-clr-expressions
  (lambda (n s1 ) (clrexp) "Ok"))

(vector-set! api-calls 3 api-call-clr-expressions)

(define api-call-get-expressions
  (lambda (n s1 ) (string-append expressions "\n")))

(vector-set! api-calls 4 api-call-get-expressions)

(define api-call-pretty
  (lambda (n s1) (string-append " " (eval->pretty s1))))

(vector-set! api-calls 5 api-call-pretty)

(define api-call-save-evaluator-text
  (lambda (n s1) (save-evaluator-text s1) "Ok"))

(vector-set! api-calls 6 api-call-save-evaluator-text)

 
 ;;;;

 
;; graphics 2d
;; 

(define draw-line
  (lambda (x y x1 y1 w)
    (let ([cmd (format
       (string-append "draw_line ( ~s, ~s, ~s, ~s, ~s)")
            x y x1 y1 w)])
      (web-eval cmd))))
