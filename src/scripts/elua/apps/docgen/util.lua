local cutil = require("cutil")
local ffi = require("ffi")

local M = {}

local doc_root, root_ns

local path_sep, rep_sep = "/", "\\"
if ffi.os == "Windows" then
    path_sep, rep_sep = rep_sep, path_sep
end

M.path_join = function(...)
    return (table.concat({ ... }, path_sep):gsub(rep_sep, path_sep))
end

M.path_to_nspace = function(p)
    return p:gsub(rep_sep, ":"):gsub(path_sep, ":"):lower()
end

M.nspace_to_path = function(ns)
    return ns:gsub(":", path_sep):gsub(rep_sep, path_sep):lower()
end

M.make_page = function(path)
    return M.path_join(doc_root, "auto", path .. ".txt")
end

M.get_root_ns = function()
    return root_ns
end

M.mkdir_r = function(dirn)
    assert(cutil.file_mkpath(M.path_join(doc_root, "auto", dirn)))
end

M.mkdir_p = function(path)
    M.mkdir_r(path:match("(.+)" .. path_sep .. "([^" .. path_sep .. "]+)"))
end

M.rm_root = function()
    cutil.file_rmrf(M.path_join(doc_root, "auto"))
end

M.init = function(root, rns)
    doc_root = root:gsub(rep_sep, path_sep)
    root_ns = rns
end

return M
