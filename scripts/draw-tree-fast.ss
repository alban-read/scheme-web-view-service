;; switch to image view; and drag to top left pane
;; then use control enter; function should create the commands
;; to draw the tree, then run them at once.
;; drag and drop a file or text.

(web-exec 
 "
 ctx.strokeStyle='green';
 function draw_line ( x, y, x1, y1, w) {
    ctx.translate(0.5, 0.5);
 	ctx.beginPath();
    ctx.moveTo(x, y);
    ctx.lineTo(x1, y1);
    ctx.lineWidth = w; 
    ctx.stroke();
 	ctx.translate(-0.5, -0.5);
 }" "")

(define draw-tree-fast
  (lambda ()
    (define *scale* 8)
    (define *split* 16)
    (define command-list '())
    (define add-draw-line
      (lambda (x y x1 y1 w)
        (let ([cmd (format
                     (string-append "draw_line ( ~s, ~s, ~s, ~s, ~s);\n") x
                     y x1 y1 w)])
          (set! command-list (cons cmd command-list)))))
    (define degrees->radians
      (lambda (d) (let ([pi 3.141592653589793]) (* d pi 1/180))))
    (define (create-tree x1 y1 angle depth)
      (if (zero? depth)
          '()
          (let ([x2 (+ x1
                       (* (cos (degrees->radians angle)) depth *scale*))]
                [y2 (- y1
                       (* (sin (degrees->radians angle)) depth *scale*))])
            (append
              (list (map truncate (list x1 y1 x2 y2 depth)))
              (create-tree x2 y2 (- angle *split*) (- depth 1))
              (create-tree x2 y2 (+ angle *split*) (- depth 1))))))
    (define tree (create-tree 350 480 90 10))
    (define draw-a-line
      (lambda (x y x1 y1 w)
        (add-draw-line x y x1 y1 (exact->inexact (/ w 2)))))
    (define get-line (lambda (x) (apply draw-a-line x)))
    (map get-line tree)
    (web-exec (apply string-append command-list) "")
    #t))

(draw-tree-fast)