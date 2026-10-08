"""Minimal ELF64 (little-endian) reader for execution-identity checks (follow-up E1).

Standard library only, so that it can also be imported inside gdb's embedded Python. It reads the
symbol tables, the PT_LOAD segments and the GNU build-id, and returns the exact file bytes of a
function, so that the code a process executed can be compared with the code in each artifact.
"""
import hashlib
import struct

_EHDR = struct.Struct("<16sHHIQQQIHHHHHH")
_PHDR = struct.Struct("<IIQQQQQQ")
_SHDR = struct.Struct("<IIQQQQIIQQ")
_SYM = struct.Struct("<IBBHQQ")
PT_LOAD, SHT_SYMTAB, SHT_DYNSYM, SHT_NOTE, NT_GNU_BUILD_ID = 1, 2, 11, 7, 3
BINDS = {0: "LOCAL", 1: "GLOBAL", 2: "WEAK", 10: "GNU_UNIQUE"}
TYPES = {0: "NOTYPE", 1: "OBJECT", 2: "FUNC", 3: "SECTION", 4: "FILE", 10: "IFUNC"}
VISIBILITY = {0: "DEFAULT", 1: "INTERNAL", 2: "HIDDEN", 3: "PROTECTED"}


def _cstr(blob, off):
    end = blob.index(b"\0", off)
    return blob[off:end].decode("utf-8", "replace")


def read_elf(path):
    """{'loads': [(vaddr, offset, filesz, memsz)], 'symbols': [...], 'build_id': hex or None}."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"\x7fELF" or data[4] != 2 or data[5] != 1:
        raise ValueError(f"{path}: not a little-endian ELF64 file")
    e = _EHDR.unpack_from(data, 0)
    phoff, shoff, phentsize, phnum, shentsize, shnum = e[5], e[6], e[9], e[10], e[11], e[12]
    loads = []
    for i in range(phnum):
        p = _PHDR.unpack_from(data, phoff + i * phentsize)
        if p[0] == PT_LOAD:
            loads.append((p[3], p[2], p[5], p[6]))
    sections = [_SHDR.unpack_from(data, shoff + i * shentsize) for i in range(shnum)]
    symbols, build_id = [], None
    for sh in sections:
        sh_type, sh_offset, sh_size, sh_link, sh_entsize = sh[1], sh[4], sh[5], sh[6], sh[9]
        if sh_type in (SHT_SYMTAB, SHT_DYNSYM) and sh_entsize:
            strtab = sections[sh_link]
            strblob = data[strtab[4]:strtab[4] + strtab[5]]
            table = "dynsym" if sh_type == SHT_DYNSYM else "symtab"
            for j in range(1, sh_size // sh_entsize):
                name_off, info, other, shndx, value, size = _SYM.unpack_from(data, sh_offset + j * sh_entsize)
                symbols.append({"name": _cstr(strblob, name_off), "value": value, "size": size,
                                "bind": BINDS.get(info >> 4, str(info >> 4)), "type": TYPES.get(info & 0xF, str(info & 0xF)),
                                "visibility": VISIBILITY.get(other & 0x3), "defined": shndx != 0, "table": table})
        elif sh_type == SHT_NOTE:
            off, end = sh_offset, sh_offset + sh_size
            while off + 12 <= end:
                namesz, descsz, ntype = struct.unpack_from("<III", data, off)
                name_start = off + 12
                desc_start = name_start + ((namesz + 3) & ~3)
                if ntype == NT_GNU_BUILD_ID and data[name_start:name_start + namesz].rstrip(b"\0") == b"GNU":
                    build_id = data[desc_start:desc_start + descsz].hex()
                off = desc_start + ((descsz + 3) & ~3)
    return {"loads": loads, "symbols": symbols, "build_id": build_id}


def find_symbol(info, name, table="dynsym"):
    """The defined symbol `name` (dynsym first, then symtab when table='any')."""
    tables = ("dynsym", "symtab") if table == "any" else (table,)
    for t in tables:
        for s in info["symbols"]:
            if s["table"] == t and s["name"] == name and s["defined"]:
                return s
    return None


def vaddr_to_offset(info, vaddr):
    for seg_vaddr, seg_off, filesz, _memsz in info["loads"]:
        if seg_vaddr <= vaddr < seg_vaddr + filesz:
            return seg_off + (vaddr - seg_vaddr)
    raise ValueError(f"virtual address {vaddr:#x} is not backed by file bytes")


def function_bytes(path, name, info=None):
    """Exact file bytes [st_value, st_value + st_size) of the defined function `name`."""
    info = info or read_elf(path)
    sym = find_symbol(info, name, "any")
    if sym is None or sym["type"] != "FUNC" or not sym["size"]:
        raise KeyError(f"{path}: no sized FUNC symbol {name!r}")
    off = vaddr_to_offset(info, sym["value"])
    with open(path, "rb") as f:
        f.seek(off)
        return f.read(sym["size"])


def function_sha256(path, name, info=None):
    return hashlib.sha256(function_bytes(path, name, info)).hexdigest()


def exported_functions(info):
    """{name: symbol} of defined GLOBAL/WEAK functions in the dynamic symbol table."""
    return {s["name"]: s for s in info["symbols"]
            if s["table"] == "dynsym" and s["defined"] and s["type"] == "FUNC" and s["bind"] in ("GLOBAL", "WEAK")}
