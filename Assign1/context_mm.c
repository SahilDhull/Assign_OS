#include<context.h>
#include<memory.h>
#include<lib.h>

#define p(str,k) printf("%s = %x\n", str, k);
#define u64 unsigned long

void allocate(struct exec_context *ctx,u64 addr,u64* k4,u32 per,u32 arg){
    // printf("start\n");
    u64 l4=0xFF8000000000,l3=0x7FC0000000,l2=0x3FE00000,l1=0x1FF000;
    u64 f=0xFFFFFFFFFF000;
    u64 *k[4];
    k[0]=k4;
    u64 pos[4]={(addr&l4)>>39,(addr&l3)>>30,(addr&l2)>>21,(addr&l1)>>12};
    u32 address,i;
    u64 val;
    for(i=0;i<3;i++){
        // printf("level %x",4-i);
        // p("okay",0);
        val=*(k[i]+pos[i]);
        // p("val",val);
        if(val!=0x0){
            val = (val&f)>>12;
            address=(u32)val;
             p("pfn",address);
            // p("i",i);
            k[i+1]=osmap(address);
            // printf("virtual %x\n",k[i+1]);
            // printf("change %x\n",*(k[i]+pos[i]) );
            *(k[i]+pos[i]) = *(k[i]+pos[i]) | per;
             printf("%x\n",*(k[i]+pos[i]) );
        }
        else{
            address=os_pfn_alloc(OS_PT_REG);
            k[i+1]=osmap(address);
             p("new address",address);
            // p("i",i);
            for(u32 j=0x0;j<0x1000;j+=0x4){
                *(k[i+1]+j)=0x0;
            }
            *(k[i]+pos[i]) = ((address<<12) & f) | per | 0x4;
		printf("%x\n",*(k[i]+pos[i]) );
        }
    }
    // p("abc",0);
    if(arg!=0x0){
        address = ctx->arg_pfn;
        u32* x = osmap(address);
        *(k[3]+pos[3]) = ((address<<12) & f) | per | 0x4;
        // p("arg",*(k[3]+pos[3]));
        return;
    }
    address=os_pfn_alloc(USER_REG);
    u32* x=osmap(address);
    for(i=0x0;i<0x1000;i+=0x4){
        *(x+i)=0x0;
    }
    *(k[3]+pos[3]) = ((address<<12) & f) | per | 0x4;
    // p("user page",*(k[3]+pos[3]));
}

void prepare_context_mm(struct exec_context *ctx)
{
    ctx->pgd=os_pfn_alloc(OS_PT_REG); //L4 page table frame number
    u64* k = osmap(ctx->pgd);
    u32 i;

    u64 code=ctx->mms[MM_SEG_CODE].start;
    u64 stack=ctx->mms[MM_SEG_STACK].end - 0x1000;
    u64 data=ctx->mms[MM_SEG_DATA].start;
    for(i=0x0;i<0x1000;i+=0x4){
        *(k+i)=0x0;
    }
    u32 per;
    per = 0x2 & ctx->mms[MM_SEG_CODE].access_flags;
    if(per==0x0) per=0x1;
    else per=0x3;
    allocate(ctx,code,k,per,0x0);
    // p("abc",0);
    per = 0x2 & ctx->mms[MM_SEG_STACK].access_flags;
    if(per==0x0) per=0x1;
    else per=0x3;
    allocate(ctx,stack,k,per,0x0);
    // p("abc",0);
    per = 0x2 & ctx->mms[MM_SEG_DATA].access_flags;
    if(per==0x0) per=0x1;
    else per=0x3;
    allocate(ctx,data,k,per,0x1);

    return;
}
void cleanup_context_mm(struct exec_context *ctx)
{
    u64 l[4]={0xFF8000000000,0x7FC0000000,0x3FE00000,0x1FF000};
    u64 f=0xFFFFFFFFFF000;
    u64 pos,val;
    u32 address;
    u64 *k=osmap(ctx->pgd);
    u64 *x=k;
    //code
    u64 addr[3]={ctx->mms[MM_SEG_CODE].start,ctx->mms[MM_SEG_STACK].end - 0x1000,ctx->mms[MM_SEG_DATA].start};
    int y[4]={39,30,21,12};
    for(int i=0;i<3;i++){
        x=k;
        for(int j=0;j<4;j++){
            pos=(addr[i]&l[j])>>y[j];
            val=x[pos];
            val=(val&f)>>12;
            address=(u32)val;
            if(j==3){
                // p("addr",address);
                os_pfn_free(USER_REG,address);
                x[pos]=0x0;
                break;
            }
            x=osmap(address);
        }
    }
    int i,j,n;
    for(n=3;n>0;n--){
        for(i=0;i<3;i++){ //code,stack,data
            x=k;
            for(int j=0;j<n;j++){ //page level
                pos=(addr[i]&l[j])>>y[j];
                val=x[pos];
                val=(val&f)>>12;
                address=(u32)val;
                if(j==n-1){
                    // printf("n = %d i = %d addr = %x\n",n,i,address);
                    if(address==0x0){
                        // printf("ohh\n" );
                        break;
                    }
                    os_pfn_free(OS_PT_REG,address);
                    // p("addr",address);
                    x[pos]=0x0;
                    break;
                }
                x=osmap(address);
            }
        }
    }
    os_pfn_free(OS_PT_REG,ctx->pgd);
    // os_pfn_free(OS_PT_REG,ctx->pgd);
    // printf("Cleaned all memory :)\n");
    return;
}
