; ══════════════════════════════════════════════════════════
;  Canvas — finger painting
;
;  class Board extends View {
;      Bitmap bmp;    // lazily created at the view's own size
;      Canvas bcv;    // draws onto bmp
;      Paint brush;
;
;      protected void onDraw(Canvas c) {
;          if (bmp == null) {
;              bmp = Bitmap.createBitmap(getWidth(), getHeight(), ARGB_8888);
;              bcv = new Canvas(bmp);
;              bcv.drawColor(BACKGROUND);
;              brush = new Paint(); ...
;          }
;          c.drawBitmap(bmp, 0, 0, null);
;      }
;
;      public boolean onTouchEvent(MotionEvent e) {
;          if (bcv != null) {
;              bcv.drawCircle(e.getX(), e.getY(), BRUSH_RADIUS, brush);
;              invalidate();
;          }
;          return true;
;      }
;  }
;
;  class CanvasDemo extends Activity {
;      public void onCreate(Bundle b) { setContentView(new Board(this)); }
;  }
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _board
    defclass "Lapp/hello/canvas/Board;", ACC_PUBLIC, "Landroid/view/View;"

    deffield "->bmp:Landroid/graphics/Bitmap;", ACC_PUBLIC
    deffield "->bcv:Landroid/graphics/Canvas;", ACC_PUBLIC
    deffield "->brush:Landroid/graphics/Paint;", ACC_PUBLIC

    method "-><init>(Landroid/content/Context;)V", ACC_PUBLIC, locals 0
        invoke_direct  "Landroid/view/View;-><init>(Landroid/content/Context;)V", p0, p1
        return_void
    end method

    ; v0=bitmap  v1=bcv/canvas  v2=width  v3=height  v4=Bitmap.Config  v5=scratch
    method "->onDraw(Landroid/graphics/Canvas;)V", ACC_PUBLIC, locals 6
        iget_object    v0, p0, "->bmp:Landroid/graphics/Bitmap;"
        if_eqz         v0, .make_bitmap
        goto           .have_bitmap

    .make_bitmap:
        invoke_virtual "Landroid/view/View;->getWidth()I", p0
        move_result     v2
        invoke_virtual "Landroid/view/View;->getHeight()I", p0
        move_result     v3
        sget_object     v4, "Landroid/graphics/Bitmap$Config;->ARGB_8888:Landroid/graphics/Bitmap$Config;"
        invoke_static   "Landroid/graphics/Bitmap;->createBitmap(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;", v2, v3, v4
        move_result_object v0
        iput_object     v0, p0, "->bmp:Landroid/graphics/Bitmap;"

        new_instance    v1, "Landroid/graphics/Canvas;"
        invoke_direct   "Landroid/graphics/Canvas;-><init>(Landroid/graphics/Bitmap;)V", v1, v0
        iput_object     v1, p0, "->bcv:Landroid/graphics/Canvas;"

        const           v5, $FFFFFFFF                           ; white paper
        invoke_virtual  "Landroid/graphics/Canvas;->drawColor(I)V", v1, v5

        new_instance    v5, "Landroid/graphics/Paint;"
        invoke_direct   "Landroid/graphics/Paint;-><init>()V", v5
        const_4         v4, 1
        invoke_virtual  "Landroid/graphics/Paint;->setAntiAlias(Z)V", v5, v4
        const           v4, $FF00C853                           ; brush colour: green
        invoke_virtual  "Landroid/graphics/Paint;->setColor(I)V", v5, v4
        iput_object     v5, p0, "->brush:Landroid/graphics/Paint;"

    .have_bitmap:
        const_4         v5, 0
        invoke_virtual  "Landroid/graphics/Canvas;->drawBitmap(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V", p1, v0, v5, v5, v5
        return_void
    end method

    ; v0=bcv/canvas  v1=x  v2=y  v3=brush paint  v4=radius  v5=return value
    method "->onTouchEvent(Landroid/view/MotionEvent;)Z", ACC_PUBLIC, locals 6
        iget_object     v0, p0, "->bcv:Landroid/graphics/Canvas;"
        if_eqz          v0, .no_canvas_yet

        invoke_virtual  "Landroid/view/MotionEvent;->getX()F", p1
        move_result     v1
        invoke_virtual  "Landroid/view/MotionEvent;->getY()F", p1
        move_result     v2
        iget_object     v3, p0, "->brush:Landroid/graphics/Paint;"
        const           v4, $41C00000                           ; radius = 24.0
        invoke_virtual  "Landroid/graphics/Canvas;->drawCircle(FFFLandroid/graphics/Paint;)V", v0, v1, v2, v4, v3
        invoke_virtual  "Landroid/view/View;->invalidate()V", p0

    .no_canvas_yet:
        const_4         v5, 1
        return          v5
    end method
end macro

macro _fields
    ; field references, unlike method references, are not
    ; auto-registered from code, so this enum constant needs
    ; spelling out
    deffield "Landroid/graphics/Bitmap$Config;->ARGB_8888:Landroid/graphics/Bitmap$Config;"
end macro

macro _activity
    defclass "Lapp/hello/canvas/CanvasDemo;", ACC_PUBLIC, "Landroid/app/Activity;"

    method "-><init>()V", ACC_PUBLIC, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 1
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1
        new_instance   v0, "Lapp/hello/canvas/Board;"
        invoke_direct  "Lapp/hello/canvas/Board;-><init>(Landroid/content/Context;)V", v0, p0
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0
        return_void
    end method
end macro

dex_lists _board, _activity, _types, _fields

dex_file
