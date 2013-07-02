
#include <math.h>
#include <fstream>
#include <cstdio>

#include <pthread.h>

#include "../../../src/runtime.h"
#include "../../../src/compiler.h"
#include "../../../src/parser.h"
#include "../../../src/library.h"
#include "../../../src/coerce.h"

#include "../../../libs/Eigen/Dense"


template<typename T>
T const& Cast(Value const& v) {
	if(v.type() != T::ValueType) _error("incorrect type passed to core function");
	return (T const&)v;
}

extern "C"
Value cat(Thread& thread, Value const* args) {
	List const& a = Cast<List>(args[0]);
	Character const& b = Cast<Character>(args[1]);
	for(int64_t i = 0; i < a.length(); i++) {
		if(!List::isNA(a[i])) {
			Character c = As<Character>(thread, a[i]);
			for(int64_t j = 0; j < c.length(); j++) {
                if(c[j] == Character::NAelement)
                    printf("NA");
                else
				    printf("%s", thread.externStr(c[j]).c_str());
				if(!(i == a.length()-1 && j == c.length()-1))
					printf("%s", thread.externStr(b[0]).c_str());
			}
		}
	}
	return Null::Singleton();
}

extern "C"
Value library(Thread& thread, Value const* args, Value& result) {
	Character from = As<Character>(thread, args[0]);
	if(from.length() > 0) {
		loadPackage(thread, "library", thread.externStr(from[0]));
	}
	return Null::Singleton();
}

extern "C"
Value readtable(Thread& thread, Value const* args) {
	Character from = As<Character>(thread, args[0]);
	Character sep_list = As<Character>(thread,args[1]);
	Character format = As<Character>(thread, args[2]);
	if(from.length() > 0 && sep_list.length() > 0 && format.length() > 0) {
		std::string name = thread.externStr(from[0]);
		std::string sep = thread.externStr(sep_list[0]);
		
		std::vector<void*> lists;
		
		for(int64_t i = 0; i < format.length(); i++) {
			if(Strings::Double == format[i] || Strings::Date == format[i]) {
				lists.push_back(new std::vector<double>);
			} else if(Strings::Character == format[i]) {
				lists.push_back(new std::vector<String>);
			}
			else if(Strings::NA !=format[i]) {
				_error("Unknown format specifier");
			}
		}
		FILE* file = fopen(name.c_str(), "r");
		if(file) {
			char buf[4096];
			for(int64_t line = 0;fgets(buf,4096,file);line++) {
				char * rest = buf;
				for(int64_t i = 0, list_idx = 0; i < format.length(); i++) {
					int sep_length = sep.length();
					char * sep_location = strstr(rest,sep.c_str());
					if(sep_location == NULL && i + 1 == format.length()) {
						sep_location = strstr(rest,"\n");
						sep_length = 1;
					}
					if(sep_location == NULL) {
						printf("line = %d, col = %d\n",(int)line, (int) (rest - buf));
						_error("Number of rows does not match format specifier");
					}
					*sep_location = '\0';
					if(Strings::Double == format[i]) {
						((std::vector<double>*)lists[list_idx])->push_back(atof(rest));
						list_idx++;
					} else if(Strings::Date == format[i]) {
						struct tm tm;
						memset(&tm,0,sizeof(struct tm));
						const char * result = strptime(rest,"%Y-%m-%d",&tm);
						if(result == NULL)
							_error("Value is not a date");
						double date = mktime(&tm);
						((std::vector<double>*)lists[list_idx])->push_back(date);
						list_idx++;
					} else if(Strings::Character == format[i]) {
						String s = thread.internStr(rest);
						((std::vector<String>*)lists[list_idx])->push_back(s);
						list_idx++;
					}
					rest = sep_location + sep_length;
				}
			}
			fclose(file);
		} else {
			_error("Unable to open file");
		}
		List l(lists.size());
		for(int64_t i = 0, list_idx = 0; i < format.length(); i++) {
			if(Strings::Double == format[i] || Strings::Date == format[i]) {
				std::vector<double> * data = (std::vector<double>*) lists[list_idx];
				Double r(data->size());
				for(uint64_t j = 0; j < data->size(); j++) {
					r[j] = (*data)[j];
				}
				l[list_idx] = r;
				delete data;
				list_idx++;
			} else if(Strings::Character == format[i]) {
				std::vector<String> * data = (std::vector<String>*) lists[list_idx];
				Character r(data->size());
				for(uint64_t j = 0; j < data->size(); j++) {
					r[j] = (*data)[j];
				}
				l[list_idx] = r;
				delete data;
				list_idx++;
			}
		}
		return l;
	} else {
		return Null::Singleton();
	}
}

Type::Enum cTypeCast(Type::Enum s, Type::Enum t)
{
	#define MEET(X, Y, Z) if(s == Type::X && t == Type::Y) return Type::Z;
	DEFAULT_TYPE_MEET(MEET);
	#undef MEET
	_error("Unexpected non-basic type in unlist (cTypeCast)");
}

// These are all tree-based reductions. Should we have a tree reduction byte code?
int64_t unlistLength(Thread& thread, int64_t recurse, Value a) {
	if(recurse > 0 && a.isList()) {
		List const& l = (List const&)a;
		int64_t t = 0;
		for(int64_t i = 0; i < l.length(); i++) 
			t += unlistLength(thread, recurse-1, l[i]);
		return t;
	}
	else if(a.isVector()) return ((Vector const&)a).length();
	else return 1;
}

Type::Enum unlistType(Thread& thread, int64_t recurse, Value a) {
	if(a.isList()) {
		List const& l = (List const&)a;
		Type::Enum t = Type::Null;
		for(int64_t i = 0; i < l.length(); i++) 
			t = cTypeCast(recurse > 0 ? unlistType(thread, recurse-1, l[i]) : l[i].type(), t);
		return t;
	}
	else if(a.isVector()) return a.type();
	else return Type::List;
}

template< class T >
void unlist(Thread& thread, int64_t recurse, Value a, T& out, int64_t& start) {
	if(recurse > 0 && a.isList()) {
		List const& l = (List const&)a;
		for(int64_t i = 0; i < l.length(); i++) 
			unlist(thread, recurse-1, l[i], out, start);
		return;
	}
	else if(a.isVector()) { Insert(thread, a, 0, out, start, ((Vector const&)a).length()); start += ((Vector const&)a).length(); }
	else _error("Unexpected non-basic type in unlist");
}

template<>
void unlist<List>(Thread& thread, int64_t recurse, Value a, List& out, int64_t& start) {
	if(recurse > 0 && a.isList()) {
		List const& l = (List const&)a;
		for(int64_t i = 0; i < l.length(); i++) 
			unlist(thread, recurse-1, l[i], out, start);
		return;
	}
	else if(a.isVector()) { Insert(thread, a, 0, out, start, ((Vector const&)a).length()); start += ((Vector const&)a).length(); }
	else out[start++] = a;
}
/*
bool unlistHasNames(Thread& thread, int64_t recurse, Value a) {
	if(a.isObject() && ((Object const&)a).hasNames()) return true;
	if(a.isObject()) a = ((Object&)a).base();
	if(recurse > 0 && a.isList()) {
		List l(a);
		bool hasNames = false;
		for(int64_t i = 0; i < l.length; i++) 
			hasNames = hasNames || unlistHasNames(thread, recurse-1, l[i]);
		return hasNames;
	}
	else return false;
}
*/
std::string makeName(Thread& thread, std::string prefix, String name, int64_t i) {
	if(prefix.length() > 0) {
		if(name != Strings::empty)
			return prefix + "." + thread.externStr(name);
		else
			return prefix + intToStr(i+1);
	}
	else {
		return thread.externStr(name);
	}
}
/*
void unlistNames(Thread& thread, int64_t recurse, Value a, Character& out, int64_t& start, std::string prefix) {
	Character names(0);
	if(a.isObject() && ((Object&)a).hasNames()) {
		names = (Character)((Object&)a).getNames();
	}
	if(a.isObject()) a = ((Object&)a).base();

	if(recurse > 0 && a.isList()) {
		List l(a);
		for(int64_t i = 0; i < l.length; i++)
			unlistNames(thread, recurse-1, l[i], out, start, makeName(thread, prefix, i < names.length ? names[i] : Strings::empty, i));
		return;
	}
	else if(a.isVector() && a.length != 1) { 
		for(int64_t i = 0; i < a.length; i++) {
			out[start++] = thread.internStr(makeName(thread, prefix, (i < names.length) ? names[i] : Strings::empty, i));
		}
	}
	else out[start++] = thread.internStr(prefix);
}
*/
// TODO: useNames parameter could be handled at the R level
extern "C"
Value unlist(Thread& thread, Value const* args) {
	Value v = args[0];
	int64_t recurse = Cast<Logical>(args[1])[0] ? std::numeric_limits<int64_t>::max() : 1;
	
	int64_t length = unlistLength(thread, recurse, v);
	Type::Enum type = unlistType(thread, recurse, v);

	switch(type) {
		#define CASE(Name) \
			case Type::Name: { \
				Name out(length); \
				int64_t i = 0; \
				unlist(thread, recurse, v, out, i); \
				return out; \
			} break;
		VECTOR_TYPES(CASE)
		#undef CASE
		default: _error("NYI: Insert into this type"); break;
	};
}

struct mapplyargs {
	List const& in;
	List& out;
	Value const& func;
};

void* mapplyheader(void* args, uint64_t start, uint64_t end, Thread& thread) {
	mapplyargs& l = *(mapplyargs*)args;
	List apply(1+l.in.length());
	apply[0] = l.func;
	for(int64_t i = 0; i < l.in.length(); i++)
		apply[i+1] = Value::Nil();
	Prototype* p = Compiler::compileTopLevel(thread, CreateCall(apply));
	return p;
}

void mapplybody(void* args, void* header, uint64_t start, uint64_t end, Thread& thread) {
	mapplyargs& l = *(mapplyargs*)args;
	Prototype* p = (Prototype*) header;
	for( size_t i=start; i!=end; ++i ) {
		for(int64_t j=0; j < l.in.length(); j++) {
			Value e;
			Element2(l.in, j, e);
			Value a;
			if(e.isVector())
				Element2(e, i % ((Vector const&)e).length(), a);
			else
				a = e;
			p->calls[0].arguments[j].v = a;
		}
		l.out[i] = thread.eval(p);
	}
	//return 0;
}

extern "C"
Value mapply(Thread& thread, Value const* args) {
	List const& x = (List const&)args[0];
	Value const& func = args[1];
	// figure out result length
	int64_t len = 1;
	for(int i = 0; i < x.length(); i++) {
		Value e;
		Element2(x, i, e);
		if(e.isVector()) 
			len = (((Vector const&)e).length() == 0 || len == 0) ? 0 : std::max(((Vector const&)e).length(), len);
	}
	List r(len);
	memset(r.v(), 0, len*sizeof(List::Element));
	thread.gcStack.push_back(r);

	/*List apply(2);
	apply[0] = func;

	// TODO: should have a way to make a simple function call without compiling,
	// or should have a fast case for compilation
	Thread ithread(state);
	for(int64_t i = 0; i < x.length; i++) {
		apply[1] = x[i];
		r[i] = eval(ithread, Compiler::compile(thread, CreateCall(apply)));
	}*/

	/*List apply(2);
	apply[0] = func;
	apply[1] = Value::Nil();
	Prototype* p = Compiler::compile(thread, CreateCall(apply));
	Thread ithread(state);
	for(int64_t i = 0; i < x.length; i++) {
		p->calls[0].arguments[0] = x[i];
		r[i] = eval(ithread, p);
	}*/

	/*pthread_t h1, h2;

	lapplyargs a1 = (lapplyargs) {0, x.length/2, thread, x, r, func};
	lapplyargs a2 = (lapplyargs) {x.length/2, x.length, thread, x, r, func};

        pthread_create (&h1, NULL, lapplybody, &a1);
        pthread_create (&h2, NULL, lapplybody, &a2);
	pthread_join(h1, NULL);
	pthread_join(h2, NULL);
	*/

	mapplyargs a1 = (mapplyargs) {x, r, func};
	thread.doall(mapplyheader, mapplybody, &a1, 0, r.length(), 1, 1); 

	thread.gcStack.pop_back();
	return r;
}

/*
void tlist(Thread& thread, Value const* args, Value& result) {
	int64_t length = args.length > 0 ? 1 : 0;
	List a = Clone(args);
	for(int64_t i = 0; i < a.length; i++) {
		a[i] = force(thread, a[i]);
		if(a[i].isVector() && a[i].length != 0 && length != 0)
			length = std::max(length, (int64_t)a[i].length);
	}
	List r(length);
	for(int64_t i = 0; i < length; i++) {
		List element(args.length);
		for(int64_t j = 0; j < a.length; j++) {
			if(a[j].isVector())
				Element2(a[j], i%a[j].length, element[j]);
			else
				element[j] = a[j];
		}
		r[i] = element;
	}
	result = r;
}
*/
extern "C"
Value source(Thread& thread, Value const* args) {
	Character file = Cast<Character>(args[0]);
	std::ifstream t(thread.externStr(file[0]).c_str());
	std::stringstream buffer;
	buffer << t.rdbuf();
	std::string code = buffer.str();

	Parser parser(thread.state);
	Value value;
	parser.execute(code.c_str(), code.length(), true, value);	
	
	return thread.eval(Compiler::compileTopLevel(thread, value));
}

extern "C"
Value environment(Thread& thread, Value const* args) {
	Value e = args[0];
	if(e.isClosure()) {
        Value result;
		REnvironment::Init(result, ((Closure const&)e).environment());
		return result;
	}
	return Null::Singleton();
}

extern "C"
void stop(Thread& thread, Value const* args, Value& result) {
	// this should stop whether or not the arguments are correct...
	std::string message = thread.externStr(Cast<Character>(args[0])[0]);
	_error(message);
	result = Null::Singleton();
}

extern "C"
void warning(Thread& thread, Value const* args, Value& result) {
	std::string message = thread.externStr(Cast<Character>(args[0])[0]);
	_warning(thread, message);
	result = Character::c(thread.internStr(message));
} 

/*void nchar_fn(Thread& thread, Value const* args, Value& result) {
	// NYI: type or allowNA
	unaryCharacter<Zip1, NcharOp>(thread, args[0], result);
}

void nzchar_fn(Thread& thread, Value const* args, Value& result) {
	unaryCharacter<Zip1, NzcharOp>(thread, args[0], result);
}*/

extern "C"
Value paste(Thread& thread, Value const* args) {
	Character a = As<Character>(thread, args[0]);
	String sep = As<Character>(thread, args[1])[0];
	std::string r = "";
	for(int64_t i = 0; i < a.length()-1; i++) {
		r += a[i];
		r += sep; 
	}
	if(0 < a.length()) r += a[a.length()-1];
	return Character::c(thread.internStr(r));
}

extern "C"
Value deparse(Thread& thread, Value const* args) {
	return Character::c(thread.internStr(thread.deparse(args[0])));
}

extern "C"
Value substitute(Thread& thread, Value const* args) {
	Value v = args[0];
	while(v.isPromise()) v = ((Closure const&)v).prototype()->expression;
	
	if(isSymbol(v)) {
		Environment* penv;
		Value const& r = thread.frame.environment->getRecursive(SymbolStr(v), penv);
		if(!r.isNil()) v = r;
		while(v.isPromise()) v = ((Closure const&)v).prototype()->expression;
	}
 	return v;
}

extern "C"
Value exists(Thread& thread, Value const* args) {
	Character c = As<Character>(thread, args[0]);
	REnvironment const& e = Cast<REnvironment>(args[1]);
	Logical l = As<Logical>(thread, args[2]);

	Environment* penv;
	Value const& v = l[0] ? e.environment()->getRecursive(c[0], penv) : e.environment()->get(c[0]);
	if(v.isNil())
		return Logical::False();
	else
		return Logical::True();
}

extern "C"
Value get(Thread& thread, Value const* args) {
	Character c = As<Character>(thread, args[0]);
	REnvironment const& e = Cast<REnvironment>(args[1]);
	Logical l = As<Logical>(thread, args[2]);

	Environment* penv;
	return l[0] ? e.environment()->getRecursive(c[0], penv) : e.environment()->get(c[0]);
}

#include <sys/time.h>

uint64_t readTime()
{
  timeval time_tt;
  gettimeofday(&time_tt, NULL);
  return (uint64_t)time_tt.tv_sec * 1000 * 1000 + (uint64_t)time_tt.tv_usec;
}

extern "C"
Value proctime(Thread& thread, Value const* args) {
	uint64_t s = readTime();
	return Double::c(s/(1000000.0));
}

extern "C"
Value traceconfig(Thread & thread, Value const* args) {
	Logical c = As<Logical>(thread, args[0]);
	if(c.length() == 0) _error("condition is of zero length");
	thread.state.epeeEnabled = Logical::isTrue(c[0]);
	return Null::Singleton();
}

// args( A, m, n, B, m, n )
extern "C"
Value matrixmultiply(Thread & thread, Value const* args) {
	double mA = asReal1(args[1]);
	double nA = asReal1(args[2]);
	Eigen::MatrixXd aa = Eigen::Map<Eigen::MatrixXd>(As<Double>(thread, args[0]).v(), mA, nA);
	
	double mB = asReal1(args[4]);
	double nB = asReal1(args[5]);
	Eigen::MatrixXd bb = Eigen::Map<Eigen::MatrixXd>(As<Double>(thread, args[3]).v(), mB, nB);

	Double c(aa.rows()*bb.cols());
	Eigen::Map<Eigen::MatrixXd>(c.v(), aa.rows(), bb.cols()) = aa*bb;
	return c;
}

// args( A, m, n )
extern "C"
Value eigensymmetric(Thread & thread, Value const* args) {
	double mA = asReal1(args[1]);
	double nA = asReal1(args[2]);
	Eigen::MatrixXd aa = Eigen::Map<Eigen::MatrixXd>(As<Double>(thread, args[0]).v(), mA, nA);
	
	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigenSolver(aa);
	Double c(aa.rows()*aa.cols());
	Eigen::Map<Eigen::MatrixXd>(c.v(), aa.rows(), aa.cols()) = eigenSolver.eigenvectors();
	Double v(aa.rows());
	Eigen::Map<Eigen::MatrixXd>(v.v(), aa.rows(), 1) = eigenSolver.eigenvalues();
	
	List r(2);
	r[0] = v;
	r[1] = c;
	return r;
}

// args( A, m, n )
extern "C"
Value eigen(Thread & thread, Value const* args) {
	/*double mA = asReal1(args[1]);
	double nA = asReal1(args[2]);
	Eigen::MatrixXd aa = Eigen::Map<Eigen::MatrixXd>(As<Double>(thread, args[0]).v(), mA, nA);
	
	Eigen::EigenSolver<Eigen::MatrixXd> eigenSolver(aa);
	Double c(aa.rows()*aa.cols());
	Eigen::Map<Eigen::MatrixXd>(c.v(), aa.rows(), aa.cols()) = eigenSolver.eigenvectors();
	Double v(aa.rows());
	//Eigen::Map<Eigen::MatrixXd>(v.v(), aa.rows(), 1) = eigenSolver.eigenvalues();
	
	List r(2);
	r[0] = v;
	r[1] = c;
	result = r;*/
	throw("NYI: eigen");
}

extern "C"
Value sort(Thread& thread, Value const* args) {
	Value a = args[0];
	if(a.isDouble()) {
		Double& r = (Double&)a;
		Resize(thread, true, r, r.length()); // just cloning
		std::sort(r.v(), r.v()+r.length());
		return r;
	}
	else if(a.isInteger()) {
		Integer& r = (Integer&)a;
		Resize(thread, true, r, r.length()); // just cloning
		std::sort(r.v(), r.v()+r.length());
	    return r;
	}
	else {
		_error("NYI: sort on this type");
	}
}

extern "C"
Value force(Thread& thread, Value const* args) {
	return args[0];
}

extern "C"
Value commandArgs(Thread& thread, Value const* args) {
	return thread.state.arguments;
}

extern "C"
Value repeat2(Thread& thread, Value const* args) {
	Integer a = Cast<Integer>(args[0]);
	int64_t len = Cast<Integer>(args[1])[0];

	return Repeat(a, len);
}

extern "C"
Value getAttributes(Thread& thread, Value const* args) {
    Value v = args[0];

    if(!v.isObject() || !((Object const&)v).hasAttributes())
        return Null::Singleton();

    Dictionary* attributes = ((Object const&)v).attributes();
    if(attributes->Size() == 0)
        return Null::Singleton();

    List result(attributes->Size());
    Character names(attributes->Size());
    
    Dictionary::const_iterator it = attributes->begin();
    for(size_t i = 0; i < attributes->Size(); i++) {
        result[i] = it.value();
        names[i] = it.string();
        ++it;
    }
    Dictionary* d = new Dictionary(1);
    d->insert(Strings::names) = names;
    result.attributes(d);
    return result;
}

extern "C"
Value emptyEnvironment(Thread& thread, Value const* args) {
    Value v;
    REnvironment::Init(v, thread.state.empty);
    return v;
}

extern "C"
Value globalEnvironment(Thread& thread, Value const* args) {
    Value v;
    REnvironment::Init(v, thread.state.global);
    return v;
}
