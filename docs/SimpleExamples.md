[index](helpindex.html)

### Simple Examples

#### Graphics

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

---

##### Draw random lines on canvas

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

--------

##### Draw Tree on canvas

```Scheme
;;
(define draw-line
  (lambda (x y x1 y1 w)
    (let ([cmd (format
            (string-append 
                 "draw_line ( ~s, ~s, ~s, ~s, ~s)")
                  x y x1 y1 w)])
      (web-eval cmd))))
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
;;
```

