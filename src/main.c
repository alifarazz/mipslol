#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAXN 1000
#define MAXMEM 1000
#define FILE_BUFFER_SIZE 1000


#define OPCODE_MASK 0x0000003F
#define FUNC_MASK 0x0000003F
#define IMM_MASK 0x0000FFFF
#define SHAMT_MASK 0x0000003F
#define REG_MASK 0x0000001F

#define OPCODE_J 0x00000002
#define OPCODE_LW 0x00000023
#define OPCODE_SW 0x0000002B
#define OPCODE_RTYPE 0x00000000
#define FUNC_ADD 0x00000020
#define FUNC_JR  0x00000008
#define FUNC_NOR 0x00000027
#define FUNC_OR  0x00000025
#define FUNC_SLT 0x0000002A
#define FUNC_SUB 0x00000022
#define FUNC_XOR 0x00000026

int32_t g_regfile[32];
int32_t g_instructions[MAXN];
int8_t *g_mainmem;

int32_t g_instructions_len;

char g_char_buf[FILE_BUFFER_SIZE];

/* enum Exceptions { */
/*   int_overflow, */
  
/* }; */

struct Context {
  int32_t *regfile;
  int8_t *mainmem;
  int32_t mainmem_size;
  int32_t prog_counter;
};

union ConverterIntChar {
  int32_t i;
  int8_t c[4];
};

inline void
lol_update_context(struct Context *ctx,
		   int32_t *regfile,
		   int8_t *mainmem,
		   int32_t mainmem_size,
		   int32_t prog_counter) {
  if (!ctx) return;
  ctx->regfile = regfile;
  ctx->mainmem = mainmem;
  ctx->mainmem_size = mainmem_size;
  ctx->prog_counter = prog_counter;
}

void* lol_malloc(size_t length, size_t size)
{
  size_t x = length * size;
  if (length != 0 && x / length != size) { /* check for overflow */
    fprintf(stderr,
	    "malloc failed.\n%s\n\tlength: %lu, size: %lu\n",
	    "Reason: size_t overflow", length, size);
    exit(EXIT_FAILURE);
  }
  void* mp = malloc(x);
  if (mp == NULL) {
    fprintf(stderr,
	    "malloc failed.\n%s\n\t length: %lu, size: %lu\n",
	    "malloc returned NULL", length, size);
    exit(EXIT_FAILURE);
  }
  return mp;
}

static int32_t
convert_str_bin(const char *g_char_buf)
{
  int i;
  int32_t res = 0x0000;

  for (i = 0; i < 8 * (int)sizeof(int32_t); i++) {
    res <<= 1;			/* add zero to right */
    if (g_char_buf[i] == '1')	/* if it's a 1 then */
      res |= 1;			/* toggle the LSB bit */
  }
  return res;
}

int32_t
lol_get_opcode(int32_t inst)
/* return opcode zerofilled to 32 bits */
{
  return (inst >> 26) & OPCODE_MASK;
}

int32_t
lol_get_rs(int32_t inst)
{
  return (inst >> 21) & REG_MASK;
}

int32_t
lol_get_rt(int32_t inst)
{
  return (inst >> 16) & REG_MASK;
}

int32_t
lol_get_rd(int32_t inst)
{
  return (inst >> 11) & REG_MASK;
}

int32_t
lol_get_shamt(int32_t inst)
{
  return (inst >> 6) & SHAMT_MASK;
}

int32_t
lol_get_func(int32_t inst)
{
  return inst & FUNC_MASK;
}

int32_t
lol_get_imm(int32_t inst)
{
  return inst & IMM_MASK;
}

int
lol_is_jtype(int32_t inst)
{
  int32_t opcode = lol_get_opcode(inst);
  /* return opcode == OPCODE_J || opcode == OPCODE_JAL; */
  return opcode == OPCODE_J;
}

int
lol_is_itype(int32_t inst)
{
  int32_t opcode = lol_get_opcode(inst);
  return opcode == OPCODE_LW || opcode == OPCODE_SW;
}

int
lol_is_rtype(int32_t inst)
{
  return lol_get_opcode(inst) == OPCODE_RTYPE;
}

int
lol_read_memory(const struct Context *ctx,
		const int32_t idx, int32_t *content)
{
  union ConverterIntChar convert_ic;
  if (!ctx || !(ctx->mainmem)) return -1;	      /* invalid context */
  if (!content) return -2;			      /* invalid content */
  if (idx+3 >= ctx->mainmem_size) return 1;	      /* out of memeory */

  /* Read 4 bytes from main memory and convert it to a 4 byte word */
  convert_ic.c[0] = (ctx->mainmem)[idx];
  convert_ic.c[1] = (ctx->mainmem)[idx + 1];
  convert_ic.c[2] = (ctx->mainmem)[idx + 2];
  convert_ic.c[3] = (ctx->mainmem)[idx + 3];
  *content = convert_ic.i;
  return 0;
}

int
lol_write_memory(struct Context *ctx, int32_t idx, int32_t content)
{
  union ConverterIntChar convert_ic;
  if (!ctx || !(ctx->mainmem)) return -1;  /* invalid context */
  if (idx+3 >= ctx->mainmem_size) return 1; /* out of memory */

  /* Write 'content' to 4 consecutive byte at main memory */
  convert_ic.i = content;
  (ctx->mainmem)[idx]     = convert_ic.c[0];
  (ctx->mainmem)[idx + 1] = convert_ic.c[1];
  (ctx->mainmem)[idx + 2] = convert_ic.c[2];
  (ctx->mainmem)[idx + 3] = convert_ic.c[3];
  return 0;
}

void
lol_exec_jtype(int32_t inst, struct Context *ctx)
{
  if (!ctx) return;		/* invalid context */

  if (lol_get_opcode(inst) == OPCODE_J) {
    /* get the address and shift by 2 */
    inst = (inst & 0x03FFFFFFF) << 2;
    /* keep 4 left-most bits of pc */
    ctx->prog_counter = ctx->prog_counter & 0xF0000000;
    /* update pc */
    ctx->prog_counter |= inst;
  }
}

void
lol_exec_itype(int32_t inst, struct Context *ctx)
{
  int32_t opcode = lol_get_opcode(inst);
  int32_t imm = lol_get_imm(inst);
  int32_t rs = lol_get_rs(inst),
    rt = lol_get_rt(inst);

  if (!ctx || !(ctx->regfile)) return; /* invalid context */

  switch (opcode)
    {
    case(OPCODE_LW): {
      /* Read 4 bytes from memory at location 
       * reg[rs] + imm and load it to reg[rt] */
      lol_read_memory(ctx, (ctx->regfile)[rs] + imm, &((ctx->regfile)[rt]));
      break;
    }
    case(OPCODE_SW): {
      /* Read content of reg[rt] and load it to 
       * 4 consecutive cells of memory located at
       * reg[rs] + imm */
      lol_write_memory(ctx, (ctx->regfile)[rs] + imm, (ctx->regfile)[rt]);
      break;
    }
    default:
      /* not implemented yet */
      break;
    }
}

void
lol_exec_rtype(int32_t inst, struct Context *ctx)
{
  int32_t func = lol_get_func(inst);
  int32_t rs = lol_get_rs(inst),
    rt = lol_get_rt(inst),
    rd = lol_get_rd(inst);

  if (!ctx || !(ctx->regfile)) return;

  switch (func)
    {
    case(FUNC_ADD): {
      /* @TODO : handle int overflow */
      (ctx->regfile)[rd] = (ctx->regfile)[rs] + (ctx->regfile)[rt];
      break;
    }
    case(FUNC_JR): {
      ctx->prog_counter = (ctx->regfile)[rs];
      break;
    }
    case(FUNC_NOR): {
      (ctx->regfile)[rd] = ~((ctx->regfile)[rs] | (ctx->regfile)[rt]);
      break;
    }
    case(FUNC_OR): {
      (ctx->regfile)[rd] = (ctx->regfile)[rs] | (ctx->regfile)[rt];
      break;
    }
    case(FUNC_SLT): {
      (ctx->regfile)[rd] = ((ctx->regfile)[rs] < (ctx->regfile)[rt]) ? 1:0;
      break;
    }
    case(FUNC_SUB): {
      /* @TODO : handle int overflow */
      (ctx->regfile)[rd] = (ctx->regfile)[rs] - (ctx->regfile)[rt];
      break;
    }
    case(FUNC_XOR): {
      (ctx->regfile)[rd] = (ctx->regfile)[rs] ^ (ctx->regfile)[rt];
      break;
    }
    default:
      /* not implemented yet */
      break;
    }
}

int main()
{
  int i;
  int32_t pc;
  FILE *fp;
  struct Context context;

  g_mainmem = lol_malloc(MAXMEM, sizeof(int8_t));

  if ((fp = fopen("/home/luce/workspace/mipslol/src/ins.txt", "r")) == NULL) {
    perror("fopen() failed");
    goto exit_clean;
  }

  for (i = 0; fgets(g_char_buf, FILE_BUFFER_SIZE, fp) != NULL;) {
    if (g_char_buf[0] == '#')	/* it's a comment */
      continue;

    /* assume anything after 31'th char is comment */
    g_char_buf[32] = '\0';
    g_instructions[i++] = convert_str_bin(g_char_buf);
  } 
  fclose(fp);
  g_instructions_len = i;
  
  for (i = 0; i < g_instructions_len; i++) {
    sprintf(g_char_buf, "%x", g_instructions[i]);
    printf("%s\n", g_char_buf);
  }


  pc = 0;
  while (pc < g_instructions_len) {
    int32_t instruction = g_instructions[pc];
    lol_update_context(&context, g_regfile, g_mainmem, MAXMEM, pc);

    if (lol_is_jtype(instruction))
      lol_exec_jtype(instruction, &context);
    else if (lol_is_itype(instruction))
      lol_exec_itype(instruction, &context);
    else if(lol_is_rtype(instruction))
      lol_exec_rtype(instruction, &context);
    else {
      fprintf(stderr,
  	      "encounterd invalid instruction on line %d\ninstruction: 0x%x",
  	      pc, instruction);
      goto exit_clean;
      }

    /* update variables from context */
    pc = context.prog_counter;
  }

 exit_clean:
  free(g_mainmem);
  return EXIT_SUCCESS;
}
