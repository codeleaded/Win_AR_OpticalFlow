#include "/home/codeleaded/System/Static/Library/WindowEngine.h"
#include "/home/codeleaded/System/Static/Library/RLCamera.h"
#include "/home/codeleaded/System/Static/Library/ImageFilter.h"


#define OUTPUT_WIDTH    (RLCAMERA_WIDTH / 8)
#define OUTPUT_HEIGHT   (RLCAMERA_HEIGHT / 8)


RLCamera rlc;
Sprite old;
Sprite motion;
Sprite filtered;
Sprite oldfiltered;

Vec2* flow;
Rect rect;
Vec2 rect_v;

void Setup(AlxWindow* w){
    rlc = RLCamera_New("/dev/video0",RLCAMERA_WIDTH,RLCAMERA_HEIGHT);

    old = Sprite_New(OUTPUT_WIDTH,OUTPUT_HEIGHT);
    motion = Sprite_New(OUTPUT_WIDTH,OUTPUT_HEIGHT);
    filtered = Sprite_New(OUTPUT_WIDTH,OUTPUT_HEIGHT);
    oldfiltered = Sprite_New(OUTPUT_WIDTH,OUTPUT_HEIGHT);
    
    flow = (Vec2*)malloc(sizeof(Vec2) * OUTPUT_WIDTH * OUTPUT_HEIGHT);
    
    rect = Rect_New((Vec2){ OUTPUT_WIDTH * 0.5f,OUTPUT_HEIGHT * 0.5f },(Vec2){ 10.0f,10.0f });
    rect_v = (Vec2){ 0.0f,0.0f };
}
void Update(AlxWindow* w){
    Sprite captured = Sprite_Null();
    captured.img = RLCamera_Get(&rlc,&captured.w,&captured.h);
    Sprite_Resize(&captured,OUTPUT_WIDTH,OUTPUT_HEIGHT);
    
    Clear(BLACK);

    for(int i = 0;i<captured.w * captured.h;i++){
        if(i>=motion.w * motion.h || i>=old.w * old.h) continue;
        
        float filter = (Pixel_Lightness_N(captured.img[i]) - Pixel_Lightness_N(filtered.img[i])) * 0.1f;
        filtered.img[i] = Pixel_toRGBA(filter,filter,filter,1.0f);

        //float diff = F32_Abs(Pixel_Lightness_N(captured.img[i]) - Pixel_Lightness_N(old.img[i]));
        float diff = F32_Abs(filter - Pixel_Lightness_N(oldfiltered.img[i]));
        if(diff <= 0.01f) diff = 0.0f;

        motion.img[i] = Pixel_toRGBA(diff,diff,diff,1.0f);
    }

    const int patch_size = 7;
    const int search_size = 5;
    for(int i = 0;i<captured.w * captured.h;i++){
        if(i>=motion.w * motion.h || i>=old.w * old.h) continue;
        
        const int x = i % captured.w;
        const int y = i / captured.w;

        float patch_diffmax = INFINITY;
        flow[i] = (Vec2){ 0.0f,0.0f };

        for(int sy = 0;sy<search_size;sy++){
            for(int sx = 0;sx<search_size;sx++){
                const int search_centerX = x + (sx - search_size / 2);
                const int search_centerY = y + (sy - search_size / 2);
                
                float diff = 0.0f;
                for(int py = 0;py<patch_size;py++){
                    for(int px = 0;px<patch_size;px++){
                        const int patch_centerX = search_centerX + (px - patch_size / 2);
                        const int patch_centerY = search_centerY + (py - patch_size / 2);
                        const int patch_baseX = x + (px - patch_size / 2);
                        const int patch_baseY = y + (py - patch_size / 2);

                        const float patch_pixel = Pixel_Lightness_N(Sprite_Get(&captured,patch_centerX,patch_centerY));
                        const float base_pixel = Pixel_Lightness_N(Sprite_Get(&old,patch_baseX,patch_baseY));

                        diff += F32_Abs(patch_pixel - base_pixel);
                    }
                }

                if(diff <= patch_diffmax){
                    patch_diffmax = diff;
                    flow[i] = (Vec2){ search_centerX - x,search_centerY - y };
                }
            }
        }
    }

    for(int i = 0;i<captured.w * captured.h;i++){
        if(i>=motion.w * motion.h || i>=old.w * old.h) continue;
        
        float mul = Pixel_Lightness_N(motion.img[i]) > 0 ? 1.0f : 0.0f;
        flow[i].x *= mul;
        flow[i].y *= mul;
        
        old.img[i] = captured.img[i];
        oldfiltered.img[i] = oldfiltered.img[i];
    }

    rect_v = Vec2_Add(rect_v,Vec2_Mulf(flow[(int)rect.p.y * OUTPUT_WIDTH + (int)rect.p.x],100.0f * w->ElapsedTime));
    rect_v = Vec2_Mulf(rect_v,0.85f);

    rect.p = Vec2_Add(rect.p,Vec2_Mulf(rect_v,1.0f * w->ElapsedTime));

    if(rect.p.x<0.0f){
        rect.p.x = 0.0f;
        rect_v.x *= -1.0f;
    }
    if(rect.p.y<0.0f){
        rect.p.y = 0.0f;
        rect_v.y *= -1.0f;
    }
    if(rect.p.x>OUTPUT_WIDTH - rect.d.x){
        rect.p.x = OUTPUT_WIDTH - rect.d.x;
        rect_v.x *= -1.0f;
    }
    if(rect.p.y>OUTPUT_HEIGHT - rect.d.y){
        rect.p.y = OUTPUT_HEIGHT - rect.d.y;
        rect_v.y *= -1.0f;
    }

    //Sprite_Render(WINDOW_STD_ARGS,&captured,0.0f,0.0f);
    Sprite_Render(WINDOW_STD_ARGS,&captured,0.0f,0.0f);
    Sprite_Render(WINDOW_STD_ARGS,&captured,0.0f,0.0f);
    Sprite_Render(WINDOW_STD_ARGS,&captured,0.0f,0.0f);
    RenderRect(rect.p.x,rect.p.y,rect.d.x,rect.d.y,RED);
}
void Delete(AlxWindow* w){
    RLCamera_Free(&rlc);
    Sprite_Free(&old);
    Sprite_Free(&motion);
    Sprite_Free(&filtered);
    Sprite_Free(&oldfiltered);
    
    if(flow) free(flow);
    flow = NULL;
}

int main(){
    if(Create("AR - Optical Flow",OUTPUT_WIDTH,OUTPUT_HEIGHT,16,16,Setup,Update,Delete))
        Start();
    return 0;
}