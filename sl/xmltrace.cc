#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <stack>
#include <sstream>
#include <vector>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>

#include "glconf.hh"
#include "xmltrace.hh"

#ifndef SVTRACE_LITE_H
#define SVTRACE_LITE_H

#include <vector>
#include <ostream>
#include <fstream>

namespace CodeStorage {
        struct Insn;
}

/**
 * @brief Class provides method for printing trace compatible with SV-Comp format.
 */
class SVTraceLite
{

private: // private members

        int nodeNumber_;

public: // public methods

        SVTraceLite() : nodeNumber_(1)
        {}


        /*
         * @brief Method prints trace from @instrs to the @out using.
         *
         * @instrs which contains the instructions of a error trace. It gradually
         * creates trace graph and print it to @out.
         * @param[in]  instrs     Instruction included in error trace.
         * @param[out] out        Output stream where the graph is printed
         */
        void printTrace(
                        const std::vector<const CodeStorage::Insn*>&   instrs,
                        std::ostream&                                  out);
};

#endif



// XML initialization
const std::string START       = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n\
<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n\
\t<key attr.name=\"originFileName\" attr.type=\"string\" for=\"edge\" id=\"originfile\">\n\
\t\t<default>\"&lt;command-line&gt;\"</default>\n\
\t</key>\n\
\t<key attr.name=\"invariant\" attr.type=\"string\" for=\"node\" id=\"invariant\"/>\n\
\t<key attr.name=\"invariant.scope\" attr.type=\"string\" for=\"node\" id=\"invariant.scope\"/>\n\
\t<key attr.name=\"namedValue\" attr.type=\"string\" for=\"node\" id=\"named\"/>\n\
\t<key attr.name=\"nodeType\" attr.type=\"string\" for=\"node\" id=\"nodetype\">\n\
\t\t<default>path</default>\n\
\t</key>\n\
\t<key attr.name=\"isFrontierNode\" attr.type=\"boolean\" for=\"node\" id=\"frontier\">\n\
\t\t<default>false</default>\n\
\t</key>\n\
\t<key attr.name=\"isViolationNode\" attr.type=\"boolean\" for=\"node\" id=\"violation\">\n\
\t\t<default>false</default>\n\
\t</key>\n\
\t<key attr.name=\"isEntryNode\" attr.type=\"boolean\" for=\"node\" id=\"entry\">\n\
\t\t<default>false</default>\n\
\t</key>\n\
\t<key attr.name=\"isSinkNode\" attr.type=\"boolean\" for=\"node\" id=\"sink\">\n\
\t\t<default>false</default>\n\
\t</key>\n\
\t<key attr.name=\"enterLoopHead\" attr.type=\"boolean\" for=\"edge\" id=\"enterLoopHead\">\n\
\t\t<default>false</default>\n\
\t</key>\n\
\t<key attr.name=\"violatedProperty\" attr.type=\"string\" for=\"node\" id=\"violatedProperty\"/>\n\
\t<key attr.name=\"threadId\" attr.type=\"string\" for=\"edge\" id=\"threadId\"/>\n\
\t<key attr.name=\"sourcecodeLanguage\" attr.type=\"string\" for=\"graph\" id=\"sourcecodelang\"/>\n\
\t<key attr.name=\"programFile\" attr.type=\"string\" for=\"graph\" id=\"programfile\"/>\n\
\t<key attr.name=\"programHash\" attr.type=\"string\" for=\"graph\" id=\"programhash\"/>\n\
\t<key attr.name=\"specification\" attr.type=\"string\" for=\"graph\" id=\"specification\"/>\n\
\t<key attr.name=\"memoryModel\" attr.type=\"string\" for=\"graph\" id=\"memorymodel\"/>\n\
\t<key attr.name=\"architecture\" attr.type=\"string\" for=\"graph\" id=\"architecture\"/>\n\
\t<key attr.name=\"producer\" attr.type=\"string\" for=\"graph\" id=\"producer\"/>\n\
\t<key attr.name=\"sourcecode\" attr.type=\"string\" for=\"edge\" id=\"sourcecode\"/>\n\
\t<key attr.name=\"startline\" attr.type=\"int\" for=\"edge\" id=\"startline\"/>\n\
\t<key attr.name=\"startoffset\" attr.type=\"int\" for=\"edge\" id=\"startoffset\"/>\n\
\t<key attr.name=\"lineColSet\" attr.type=\"string\" for=\"edge\" id=\"lineCols\"/>\n\
\t<key attr.name=\"control\" attr.type=\"string\" for=\"edge\" id=\"control\"/>\n\
\t<key attr.name=\"assumption\" attr.type=\"string\" for=\"edge\" id=\"assumption\"/>\n\
\t<key attr.name=\"assumption.resultfunction\" attr.type=\"string\" for=\"edge\" id=\"assumption.resultfunction\"/>\n\
\t<key attr.name=\"assumption.scope\" attr.type=\"string\" for=\"edge\" id=\"assumption.scope\"/>\n\
\t<key attr.name=\"enterFunction\" attr.type=\"string\" for=\"edge\" id=\"enterFunction\"/>\n\
\t<key attr.name=\"returnFromFunction\" attr.type=\"string\" for=\"edge\" id=\"returnFrom\"/>\n\
\t<key attr.name=\"predecessor\" attr.type=\"string\" for=\"edge\" id=\"predecessor\"/>\n\
\t<key attr.name=\"successor\" attr.type=\"string\" for=\"edge\" id=\"successor\"/>\n\
\t<key attr.name=\"witness-type\" attr.type=\"string\" for=\"graph\" id=\"witness-type\"/>\n\
\n\
<graph edgedefault=\"directed\">\n\
\t<data key=\"sourcecodelang\">C</data>\n\
\t<data key=\"producer\">PredatorHP</data>\n\
\t<data key=\"specification\">__SPECIFICATION__</data>\n\
\t<data key=\"programhash\">__SRCFILEHASH__</data>\n\
\t<data key=\"memorymodel\">precise</data>\n\
\t<data key=\"architecture\">__ARCHITECTURE__</data>\n";


const std::string END               = "</graph>\n</graphml>";
const std::string PROG_FILE         = "<data key=\"programfile\">";
const std::string WITNESS_TYPE      = "<data key=\"witness-type\">";
const std::string CORRECT_WITNESS   = "correctness_witness";
const std::string VIOLATION_WITNESS = "violation_witness";
const std::string DATA_START        = "<data key=";
const std::string DATA_END          = "</data>\n";
const std::string NODE_START        = "<node id=";
const std::string NODE_END          = "</node>";
const std::string NODE_ENTRY        = "<data key=\"entry\">true</data>";
const std::string NODE_NAME         = "A";
const std::string VIOLATION         = "<data key=\"violation\">true";
const std::string EDGE_START        = "<edge source=";
const std::string EDGE_END          = "</edge>";
const std::string EDGE_TARGET       = " target=";


#if 0
bool instrsEq( const CodeStorage::Insn*                       instr1,
               const CodeStorage::Insn*                       instr2)
{
        return instr1->loc.line == instr2->loc.line &&
                instr1->loc.column >= instr2->loc.column && // TODO: remove this if needed
                instr1->code == instr2->code;
}
#endif

void printStart(
                std::ostream&                            out,
                const std::string                        filename,
                const bool                               violationWitness=true)
{
        out << START;
        out << '\t' << PROG_FILE << filename << DATA_END;
        out << '\t' << WITNESS_TYPE
            << (violationWitness ? VIOLATION_WITNESS : CORRECT_WITNESS) << DATA_END;
}


void printEnd( std::ostream&                                  out,
               int                                            endNode)
{
        out << "\t" << NODE_START << "\"" << NODE_NAME << endNode << "\">\n";
        out << "\t\t" << VIOLATION << DATA_END;
        out << "\t" << NODE_END << "\n";
        out << END << '\n';
}

void printEdge(
                std::ostream&                                  out,
                const int                                      lineNumber,
                const int                                      nodeNumber)
{
        const std::string indent("\t\t");
        out << "\t" << EDGE_START << "\"" << NODE_NAME << nodeNumber << "\"" 
            << EDGE_TARGET << "\""<<  NODE_NAME << nodeNumber+1 << "\">\n";

        out << indent << DATA_START << "\"startline\">"<< lineNumber << DATA_END;
        out << "\t" << EDGE_END << "\n";

}

void printNode(std::ostream&  out, const int nodeNumber)
{
        if (nodeNumber == 1)
        {
                out << "\t" << NODE_START << "\"" << NODE_NAME << nodeNumber << "\">\n\t\t"
                    << NODE_ENTRY << "\n\t" << NODE_END <<"\n";
        }
        else
        {
                out << "\t" << NODE_START << "\"" << NODE_NAME << nodeNumber << "\"/>\n";
        }
}



void SVTraceLite::printTrace(
                        const std::vector<const CodeStorage::Insn*>&   /*instrs*/,
                        std::ostream&                                  /*out*/)
{
#if 0   // removed
        printStart(out);

        for (size_t i = 0; i < instrs.size(); ++i)
        {
                const auto instr = instrs.at(i);
                const int lineNumber = instr->loc.line;

                if (i+1 < instrs.size() && instrsEq(instr, instrs.at(i+1)))
                {
                        continue;
                }

                printNode(out, nodeNumber_);
                printEdge(out, instr->loc.file, lineNumber, nodeNumber_);

                ++nodeNumber_;
        }

        printEnd(out, nodeNumber_);
#endif
}

int XMLTraceFlag = 0;

struct XMLTraceItem {
    const char *file;
    int line;
    std::string msg;
    XMLTraceItem():
        file(NULL),
        line(0),
        msg("")
    {}
    XMLTraceItem(const char *_file, int _line, std::string _msg):
        file(_file),
        line(_line),
        msg(_msg)
    {}
};

std::stack<XMLTraceItem> XMLTraceStack;

// this should be called after each CL_ERROR_MSG for XML generation start
void XMLTraceBegin(const char *file, int line, std::string msg) {
  if (!XMLTraceStack.empty()) // enable error recovery
    while(XMLTraceFlag) {
      XMLTraceStack.pop();
      --XMLTraceFlag;
    }

  if(XMLTraceFlag)
    return;
  if(GlConf::data.xmlTrace.empty())     // no XML file parameter specified
    return;
  //XMLTraceStack.clear();
  XMLTraceStack.push(XMLTraceItem(file,line,msg));

  XMLTraceFlag++;
}

void XMLTraceNode(const char *file, int line, std::string msg) {
  if(XMLTraceFlag!=1)
    return;
  XMLTraceStack.push(XMLTraceItem(file,line,msg));
}

// called after ERR trace dump
void XMLTraceEnd() {
  if(XMLTraceFlag!=1)
    return;
  if(XMLTraceStack.size()==0)
    return;

  std::ofstream xmlfile(GlConf::data.xmlTrace);
  if(!xmlfile)
    return;

  std::ofstream &out = xmlfile; // TODO

  int nodeNumber = 1;
  int oldLine = 0;
  XMLTraceItem i;

  i = XMLTraceStack.top();

  // XML header
  printStart(out, i.file);

  do {
        i = XMLTraceStack.top();
        XMLTraceStack.pop();
        //xmlfile << "TRACE: " << i.file << "  " << i.line << "\n";

        // no duplicities
        if (oldLine==i.line) continue;
        oldLine=i.line;

        printNode(out, nodeNumber);
        printEdge(out, i.line, nodeNumber);
        ++nodeNumber;
  } while (XMLTraceStack.size()>1);

  // the last one:
  i = XMLTraceStack.top();
  XMLTraceStack.pop();
  //xmlfile << "TRACE-ERROR: " << i.file << "  " << i.line << "\n";
  if (i.line>0) {
    printNode(out, nodeNumber);
    printEdge(out, i.line, nodeNumber);
    ++nodeNumber;
  }

  // XML footer
  printEnd(out, nodeNumber);

  XMLTraceFlag++;
}
