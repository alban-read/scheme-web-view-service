[index](helpindex.html)

### Simple Examples

#### Hello World

Hello world as an expression

```Scheme
"hello world"
```

Display hello world

```Scheme
(display "hello world\n")
```

Request name; then say hello.

```Scheme
;;
 (define say-hi
  (lambda (name)
    (web-eval (string-append "alert('hello " name "');"))))
;; will call say-hi
(web-exec "prompt('What is your name?');" "say-hi") 
;;
```









-----

### Numbers

```Scheme 
(+ 1 2)
```

Should result in 3.

```Scheme
(* (+ 1.0 (* 7.2 5.5)))
```

Should result in 40.6





 