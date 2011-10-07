function parse_log(data)
	pos = string.find(data, " ")
	logname = string.sub(data, 1, pos)
	logcontent = string.sub(data, pos, -1)
	pos = string.find(logname, '\\')
	print(data)
end
