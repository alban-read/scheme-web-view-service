[index](helpindex.html)

### Simple Examples

#### 2d Graphics

------------

##### Draw the letter M

```Scheme
;;
;; example send 10 separate commands to draw the letter M.
(web-eval "ctx.fillStyle = 'green';" )
(web-eval "ctx.fillRect(0, 0, canvas.width, canvas.height);" )
(web-eval "ctx.beginPath();" )
(web-eval "ctx.moveTo(90, 130);")
(web-eval "ctx.lineTo(95, 25);")
(web-eval "ctx.lineTo(150, 80);")
(web-eval "ctx.lineTo(205, 25);")
(web-eval "ctx.lineTo(210, 130);")
(web-eval "ctx.lineWidth = 15;")
(web-eval "ctx.stroke();")
(display "finished M")
(newline)
;;
;;
```

*This tests that messages are not lost; and also that they arrive in the right order.*

---

##### Draw line is defined in the base script

```Scheme
;;
;; using web eval; draw line
(define draw-line
  (lambda (x y x1 y1 w)
    (let ([cmd (format
      (string-append "draw_line ( ~s, ~s, ~s, ~s, ~s)")
          x y x1 y1 w)])
      (web-eval cmd))))
;;
```

##### Draw random lines on canvas

```Scheme

;;
;; 100 random lines.
(dotimes
  100
  (web-eval
    "ctx.strokeStyle = '#'+((1<<24)*Math.random()|0).toString(16);")
  (draw-line (random 1000) (random 1000) (random 1000)
    (random 1000) (random 12)))
;;
;;
```

*This is a good test of the communications channel.*

--------

##### Draw Tree on canvas

```Scheme
;;
(define draw-tree
 (lambda ()
	(define *scale* 8) 
	(define *split* 16)  
	(define degrees->radians 
	 (lambda (d)
		(let [ (pi 3.1415926535897932384626433832795028841)]
				(* d pi 1/180))))
	(define (create-tree x1 y1 angle depth)
	  (if (zero? depth)
		'()
		(let ([x2 (+ x1 (* (cos 
              (degrees->radians angle)) depth *scale*))]
			    [y2 (- y1 (* 
              (sin (degrees->radians angle)) depth *scale*))])
		  (append (list (map truncate (list x1 y1 x2 y2 depth)))
				  (create-tree x2 y2 
                     (- angle *split*) (- depth 1))
				  (create-tree x2 y2 
                     (+ angle *split*) (- depth 1))))))

	(define tree (create-tree 350 480 90 10))
	
	(define draw-a-line 
		(lambda (x y x1 y1 w) 
		  (draw-line 
			x y x1 y1 (/ w 2))))

	(define get-line 
	  (lambda (x) 
		(apply draw-a-line x)))
   
	(map get-line tree) #t))
;;
(draw-tree)
;;
```

----

#### Draw Fern 

*First select and run entire; then select and run function fern function repeatedly.*

```Scheme

;; fractal fern 
(web-eval "ctx.fillStyle = 'green';")

(define x 250.0)
(define y 200.0)
(define x0 0.0)
(define y0 0.0)

(define transform
  (lambda ()
    (set! x0 x)
    (set! y0 y)
    (let ([r (random 100)])
      (cond
        [(< r 1) 
         (begin 
           (set! x 0.0) 
           (set! y (* 0.16 y0)))]
        [(< r 86)
         (begin
           (set! x (+ (* 0.85 x0) (* 0.04 y0)))
           (set! y (+ (* -0.04 x0) (* 0.85 y0) 1.6)))]
        [(< r 93)
         (begin
           (set! x (+ (* 0.2 x0) (* -0.26 y0)))
           (set! y (+ (* 0.23 x0) (* 0.22 y0) 1.6)))]
        [else
         (begin
           (set! x (+ (* -0.15 x0) (* 0.28 y0)))
           (set! y (+ (* 0.26 x0) (* 0.24 y0) 0.44)))]))))

(define draw-point
  (lambda ()
    (fill-rect (+ (* x 50) 200)
               (- 500 (- (* y 50) 10))
               1 1)))

(define fern
  (lambda () 
    (dotimes 100 
     (transform) (draw-point))))

;; repeat fern
;;  
(fern)
;;
```

Repeat (fern) until the image appears.

*filling a rectangle of 1,1 is the fastest way to draw repeated points of the same colour.*

---



