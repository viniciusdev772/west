package vdev.com.android.support;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.View;

public class ESPView extends View {
    public interface Renderer {
        void draw(ESPView view, Canvas canvas);
    }

    public static int FPS = 60;

    private static volatile Renderer renderer;

    private final Paint filledPaint = new Paint();
    private final Paint strokePaint = new Paint();
    private final Paint textPaint = new Paint();
    private final Runnable frameTicker = new Runnable() {
        @Override
        public void run() {
            if (!rendering) {
                return;
            }
            postInvalidateOnAnimation();
        }
    };
    private boolean rendering = false;

    private native void DrawOn(Canvas canvas);

    public ESPView(Context context) {
        this(context, null, 0);
    }

    public ESPView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ESPView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initializePaints();
        setFocusable(false);
        setFocusableInTouchMode(false);
        setClickable(false);
        setBackgroundColor(Color.TRANSPARENT);
    }

    public static void setRenderer(Renderer nextRenderer) {
        renderer = nextRenderer;
    }

    public static void clearRenderer() {
        renderer = null;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (canvas == null || getVisibility() != VISIBLE) {
            return;
        }

        clearCanvas(canvas);

        DrawOn(canvas);

        Renderer activeRenderer = renderer;
        if (activeRenderer != null) {
            activeRenderer.draw(this, canvas);
        }

        if (rendering) {
            postOnAnimation(frameTicker);
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        startRendering();
    }

    public void shutdown() {
        stopRendering();
    }

    @Override
    protected void onDetachedFromWindow() {
        shutdown();
        super.onDetachedFromWindow();
    }

    private void startRendering() {
        if (rendering) {
            return;
        }
        rendering = true;
        postOnAnimation(frameTicker);
    }

    private void stopRendering() {
        rendering = false;
        removeCallbacks(frameTicker);
    }

    private void initializePaints() {
        strokePaint.setStyle(Paint.Style.STROKE);
        strokePaint.setAntiAlias(true);
        strokePaint.setColor(Color.BLACK);

        filledPaint.setStyle(Paint.Style.FILL);
        filledPaint.setAntiAlias(true);
        filledPaint.setColor(Color.BLACK);

        textPaint.setStyle(Paint.Style.FILL_AND_STROKE);
        textPaint.setAntiAlias(true);
        textPaint.setColor(Color.BLACK);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setStrokeWidth(1.1f);
    }

    public void clearCanvas(Canvas canvas) {
        canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
    }

    public void drawLine(Canvas canvas, int a, int r, int g, int b, float lineWidth, float fromX, float fromY, float toX, float toY) {
        strokePaint.setColor(Color.rgb(r, g, b));
        strokePaint.setAlpha(a);
        strokePaint.setStrokeWidth(lineWidth);
        canvas.drawLine(fromX, fromY, toX, toY, strokePaint);
    }

    public void drawText(Canvas canvas, int a, int r, int g, int b, String text, float posX, float posY, float size) {
        textPaint.setColor(Color.rgb(r, g, b));
        textPaint.setAlpha(a);
        textPaint.setTextSize(resolveTextSize(size));
        canvas.drawText(text, posX, posY, textPaint);
    }

    public void drawCircle(Canvas canvas, int a, int r, int g, int b, float stroke, float posX, float posY, float radius) {
        strokePaint.setColor(Color.rgb(r, g, b));
        strokePaint.setAlpha(a);
        strokePaint.setStrokeWidth(stroke);
        canvas.drawCircle(posX, posY, radius, strokePaint);
    }

    public void drawFilledCircle(Canvas canvas, int a, int r, int g, int b, float posX, float posY, float radius) {
        filledPaint.setColor(Color.rgb(r, g, b));
        filledPaint.setAlpha(a);
        canvas.drawCircle(posX, posY, radius, filledPaint);
    }

    public void drawRect(Canvas canvas, int a, int r, int g, int b, float stroke, float x, float y, float width, float height) {
        strokePaint.setStrokeWidth(stroke);
        strokePaint.setColor(Color.rgb(r, g, b));
        strokePaint.setAlpha(a);
        canvas.drawRect(x, y, x + width, y + height, strokePaint);
    }

    public void drawFilledRect(Canvas canvas, int a, int r, int g, int b, float x, float y, float width, float height) {
        filledPaint.setColor(Color.rgb(r, g, b));
        filledPaint.setAlpha(a);
        canvas.drawRect(x, y, x + width, y + height, filledPaint);
    }

    private float resolveTextSize(float baseSize) {
        DisplayMetrics metrics = getResources().getDisplayMetrics();
        int maxDimension = Math.max(metrics.widthPixels, metrics.heightPixels);
        if (maxDimension > 1920) {
            return baseSize + 4.0f;
        }
        if (maxDimension == 1920) {
            return baseSize + 2.0f;
        }
        return baseSize;
    }
}
