lxe = {
    rollAttribute = function (tagId, attributeName, attributeValues)
        local l = document:getElementById(tagId);
        local attributeValue = l:getAttribute(attributeName)
        local index = lxe.indexOf(attributeValues, attributeValue)
        if(index==nil) then
            l:setAttribute(attributeName, attributeValues[1])
        else
            local newIndex=(index+1) % (#attributeValues)
            l:setAttribute(attributeName, attributeValues[newIndex+1])
        end
    end,
    indexOf = function (array, value)
        for i, v in ipairs(array) do
            if v == value then
                return i-1
            end
        end
        return nil
    end,
    inspect = function (o, indent)
        if indent == nil then indent = 0 end
        local indent_str = string.rep("    ", indent)
        local output_it = function(str)
            print(indent_str..str)
        end

        local length = 0

        local fu = function(k, v)
            length = length + 1
            if type(v) == "userdata" or type(v) == 'table' then
                output_it(indent_str.."["..k.."]")
                lxe.inspect(v, indent+1)
            else
                output_it(indent_str.."["..k.."] "..tostring(v))
            end
        end

        local loop_pairs = function()
            for k,v in pairs(o) do fu(k,v) end
        end

        local loop_metatable_pairs = function()
            for k,v in pairs(getmetatable(o)) do fu(k,v) end
        end

        if not pcall(loop_pairs) and not pcall(loop_metatable_pairs) then
            output_it(indent_str.."[[??]]")
        else
            if length == 0 then
                output_it(indent_str.."{}")
            end
        end
    end,

    newInheritedTable = function(baseTable)
        o = {__index = baseTable}
        setmetatable(o, baseTable)
        return o
    end,
    
    DomElementPrototype = {
        getAttribute = LuaWrapperFFI.DomElementPrototype_getAttribute,
        setAttribute = LuaWrapperFFI.DomElementPrototype_setAttribute,
        hasAttribute = LuaWrapperFFI.DomElementPrototype_hasAttribute
     --   createElement = LuaWrapperFFI.ffi_DomElementPrototype_createElement,
     --   remove = LuaWrapperFFI.ffi_DomElementPrototype_remove
    }
}

lxe.DomElementPrototype.__index = lxe.DomElementPrototype


document = {
    getElementById = LuaWrapperFFI.Document_getElementById
}

