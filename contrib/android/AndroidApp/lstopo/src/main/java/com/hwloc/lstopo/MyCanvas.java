package com.hwloc.lstopo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.View;
import android.widget.RelativeLayout;

// Create a canvas in which we can draw a line
public class MyCanvas extends View {

    Paint paint;

    public MyCanvas(Context context, float x1, float x2, float y1, float y2){
        super(context);

        paint = new Paint();
        paint.setColor(Color.BLACK);
        paint.setStrokeWidth(10);

        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams((int)(x2 - x1) + 10, (int)(y2 - y1) + 10);
        setLayoutParams(params);
        setY(y1);
        setX(x1);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.drawLine(0, 0, getWidth() - 10, getHeight() - 10, paint);
    }
}
