#ifndef __INC_MCSV_H__
#define __INC_MCSV_H__
#ifndef NULL
#define NULL							0
#define MCSV_NULL
#endif

#ifndef _T
#define _T(s) s
#define MCSV_T
#endif

#define MCSV_VERSION			0x010B
#define MCSV_DEF_BUFFER			8192
#define MCSV_DEF_QUOTW			L'\x22'
#define MCSV_DEF_SEPW			L"\x2c"
#define MCSV_DEF_QUOTA			'\x22'
#define MCSV_DEF_SEPA			"\x2c"
#define MCSV_MSG_BADALLOC		_T("Bad alloc")
#define MCSV_MSG_OUTOFINDEX		_T("Out of index")
#define MCSV_MSG_NOCHANGE		_T("No change")

template<typename T>
class MCSVParser{
protected:
	int 			m_bufsize;
	int				m_colums;
	T				m_quot;
	T*				m_buf;
	T*				m_sep;
	T**				m_flds;
	T*				m_quots;
	bool			m_copy;
	T				m_empty;
	bool			m_quotstate;
	
	int _s_len(const T* str)const
	{
		const T* p;
		int cnt;
		for(cnt=0,p=str; *p; ++p) ++cnt;
		return cnt;
	}
	void _s_buf_alloc(int size)
	{
		if(size > m_bufsize){
			_s_buf_free();
			try{ m_buf = new T[size+1]; }
			catch(...){ m_buf = NULL; throw MCSV_MSG_BADALLOC; }
			m_bufsize = size;
		}else{
			_s_buf_free(false);
		}
	}
	void _s_buf_free(bool bf=true)
	{
		_s_fld_free();
		if(bf){
			delete [] m_buf;
			m_buf = NULL;
			m_bufsize = 0;
		}
		m_flds = NULL;
		m_quots = NULL;
	}
	void _s_sep_alloc(const T* sep)
	{
		delete [] m_sep;
		try{ m_sep = new T[_s_len(sep)+1]; }
		catch(...){ m_sep = NULL; throw MCSV_MSG_BADALLOC; }
		_s_copy(m_sep, sep);
	}
	void _s_sep_free()
	{
		delete [] m_sep;
		m_sep = NULL;
	}
	void _s_fld_alloc(int count)
	{
		_s_fld_free();
		try{ m_flds = new T*[count]; }
		catch(...){ m_flds = NULL; throw MCSV_MSG_BADALLOC; }
		try{ m_quots = new T[count]; }
		catch(...){ m_quots = NULL; throw MCSV_MSG_BADALLOC; }
		m_colums = count;
	}
	void _s_fld_free()
	{
		delete [] m_flds;
		delete [] m_quots;
		m_colums = 0;
		m_flds = NULL;
		m_quots = NULL;
	}
	void _s_trimcrlf()
	{
		T* p;
		if(m_buf != NULL){
			for(p=m_buf+_s_len(m_buf); p>=m_buf && (!*p||*p==_T('\r')||*p==_T('\n')); --p) *p = 0;
		}
	}
	bool _s_isequal(const T* str1, const T* str2, int len=0) const
	{
		const T *p1, *p2;
		int cnt = 0;
		for(p1=str1,p2=str2; *p1 && *p2 && *p1==*p2; ++p1,++p2){
			++cnt;
			if(len && cnt >= len) break;
		}
		return (*p1 == *p2);
	}
	T* _s_copy(T* dest, const T* src, int len=0) const
	{
		T* pd;
		const T* ps;
		int cnt;
		
		for(pd=dest,ps=src,cnt=0; *ps; ++cnt,++pd,++ps){
			if(len && cnt >= len) break;
			*pd = *ps;
		}
		if(!len || cnt < len) *pd = 0;
		return dest;
	}
	int _s_colcount() const
	{
		const T* p;
		bool qf = false, tf = true, quots = false;
		int count = 0;
		int seplen;
		
		if(m_buf == NULL) return count;
		count = 1;
		if(m_sep == NULL) return count;
		seplen = _s_len(m_sep);
		if(seplen < 1) return count;
		
		for(p=m_buf; *p; ++p){
			if(*p == m_quot){
				if(tf){
					if(quots){
						tf = false;
						qf = !qf;
					}else{
						qf = true;
						quots = true;
					}
				}else if(quots){
					++p;
					if(!(*p)) break;
					if(*p != m_quot) qf = !qf;
				}
			}else{
				if(tf) tf = false;
			}
			if(!qf && _s_isequal(p,m_sep,seplen)){
				++count;
				quots = false;
				tf = true;
				p += (seplen-1);
			}
		}
		
		return count;
	}
	int _s_split(int colums=0)
	{
		T* p;
		int i, cnt, ret = 0;
		bool qf = false, tf = true;
		int seplen;
		
		if(colums) cnt = colums;
		else cnt = _s_colcount();
		
		if(cnt < 1) return ret;
		seplen = _s_len(m_sep);
		_s_fld_alloc(cnt);
		for(i=0; i<cnt; ++i){
			m_quots[i] = 0;
			m_flds[i] = &m_empty;
		}
		for(p=m_buf; *p; ++p){
			if(*p == m_quot){
				if(tf){
					if(m_quots[ret]){
						m_flds[ret] = p;
						tf = false;
						qf = !qf;
					}else{
						qf = true;
						m_quots[ret] = m_quot;
					}
				}else if(m_quots[ret]){
					_s_copy(p, p+1);
					if(*p != m_quot) qf = !qf;
				}
			}else{
				if(tf){
					m_flds[ret] = p;
					tf = false;
				}
			}
			
			if(!qf && _s_isequal(p,m_sep,seplen)){
				if(m_quots[ret] && *(p-1) == m_quot && (p-1) == m_flds[ret]) m_flds[ret] = p;
				++ret;
				if(ret >= cnt){
					if(qf){
						while(*p){
							if(*p == m_quot){
								_s_copy(p, p+1);
								if(*p != m_quot) qf = !qf;
							}
							++p;
						}
					}
					break;
				}
				*p = 0;
				tf = true;
				p += (seplen-1);
			}
		}
		if(!qf && *p == 0){
			if(m_quots[ret] && *(p-1) == m_quot && (p-1) == m_flds[ret]) m_flds[ret] = p;
		}
		m_quotstate = !qf;
		return (ret+1);
	}
public:
	MCSVParser() : m_bufsize(0),
					m_colums(0),
					m_quot(sizeof(T) > 1 ? MCSV_DEF_QUOTW : MCSV_DEF_QUOTA),
					m_buf(NULL),
					m_sep(NULL),
					m_flds(NULL),
					m_quots(NULL),
					m_copy(false),
					m_empty(0),
					m_quotstate(true)
	{
		try{
			_s_buf_alloc(MCSV_DEF_BUFFER);
			_s_sep_alloc(sizeof(T) > 1 ? ((T*)MCSV_DEF_SEPW) : ((T*)MCSV_DEF_SEPA));
			m_buf[0] = 0;
		}catch(...){
			_s_buf_free();
			_s_sep_free();
			throw MCSV_MSG_BADALLOC;
		}
	}
	virtual ~MCSVParser()
	{
		if(!m_copy){
			_s_buf_free();
			_s_sep_free();
		}
	}
	MCSVParser(const MCSVParser<T>& csvparser) :
					m_bufsize(0),
					m_colums(0),
					m_quot(0),
					m_buf(NULL),
					m_sep(NULL),
					m_flds(NULL),
					m_quots(NULL),
					m_copy(true),
					m_empty(0),
					m_quotstate(true)
	{
		m_bufsize = csvparser.m_bufsize;
		m_colums = csvparser.m_colums;
		m_quot = csvparser.m_quot;
		m_buf = csvparser.m_buf;
		m_sep = csvparser.m_sep;
		m_flds = csvparser.m_flds;
		m_quots = m_quots;
		m_copy = true;
	}
	void alloc(int size)
	{
		if(m_copy) throw MCSV_MSG_NOCHANGE;
		_s_buf_free();
		_s_buf_alloc(size);
	}
	void setrecord(const T* rec, int colums=0)
	{
		if(m_copy) throw MCSV_MSG_NOCHANGE;
		if(rec == NULL) _s_buf_free();
		else{
			_s_buf_alloc(_s_len(rec));
			_s_copy(m_buf,rec);
			if(*rec) m_buf[_s_len(rec)] = 0;
			else m_buf[0] = 0;
			_s_trimcrlf();
			_s_split(colums);
		}
	}
	void setsep(const T* sep)
	{
		if(m_copy) throw MCSV_MSG_NOCHANGE;
		if(sep == NULL) return;
		else _s_sep_alloc(sep);
	}
	void setquot(T quot){ m_quot = quot; }
	int getcolums() const { return m_colums; }
	int getcol(T* buf, int len, int idx) const
	{
		if(idx < 0 || idx >= m_colums) throw MCSV_MSG_OUTOFINDEX;
		if(len > 0 && m_flds != NULL){
			_s_copy(buf, m_flds[idx], len);
			buf[len-1] = 0;
			return _s_len(m_flds[idx]);
		}
		return 0;
	}
	T getquot(int idx) const
	{
		if(idx < 0 || idx >= m_colums) throw MCSV_MSG_OUTOFINDEX;
		if(m_quots != NULL) return m_quots[idx];
		return 0;
	}
	T getsettingquot() const { return m_quot; }
	const T* getsettingsep() const { return m_sep; }
	bool getsplitstate() const { return m_quotstate; }
	const T* operator[](int idx) const
	{
		if(idx < 0 || idx >= m_colums) throw MCSV_MSG_OUTOFINDEX;
		if(m_flds != NULL) return m_flds[idx];
		return NULL;
	}
	const MCSVParser& operator=(const MCSVParser<T>& csvparser)
	{
		if(!m_copy){
			_s_buf_free();
			_s_sep_free();
		}
		m_bufsize = csvparser.m_bufsize;
		m_colums = csvparser.m_colums;
		m_quot = csvparser.m_quot;
		m_buf = csvparser.m_buf;
		m_sep = csvparser.m_sep;
		m_flds = csvparser.m_flds;
		m_quots = m_quots;
		m_copy = true;
		return (*this);
	}
};


#ifdef MCSV_NULL
#undef NULL
#undef MCSV_NULL
#endif

#ifdef MCSV_T
#undef _T
#undef MCSV_T
#endif

#endif		// __INC_MCSV_H__
