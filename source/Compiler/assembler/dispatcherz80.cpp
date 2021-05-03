#include "dispatcherz80.h"


ASTdispatcherZ80::ASTdispatcherZ80()
{
    // Override with Z80 values!
    m_jmp = "jp ";
    m_mov = "ld ";
    m_cmp = "cp ";
    m_jne = "jr nz,";
}

/*
 * Methods than handle all 16 bit SHR and SHL operations
 *
 */
void ASTdispatcherZ80::Handle16bitShift(QSharedPointer<NodeBinOP> node)
{   node->m_left->setForceType(TokenType::INTEGER);
    node->m_left->Accept(this);
//    as->Asm("ld l,a");
//    as->Asm("ld h,0");
    // sorry!
    if (!node->m_right->isPureNumeric())
        ErrorHandler::e.Error("Only constant 16-bit shifts are supported", node->m_op.m_lineNumber);

    int val = node->m_right->getValueAsInt(as);
    // Shl: simply add hl N times
    if (node->m_op.m_type == TokenType::SHL) {
        for (int i=0;i<val;i++)
            as->Asm("add hl,hl");
    }
    if (node->m_op.m_type == TokenType::SHR) {
        for (int i=0;i<val;i++) {
            as->Asm("srl h");
            as->Asm("rr l");
        }
    }
}

void ASTdispatcherZ80::AssignString(QSharedPointer<NodeAssign> node, bool isPointer) {

    QSharedPointer<NodeString> right = qSharedPointerDynamicCast<NodeString>(node->m_right);
    QSharedPointer<NodeVar> left = qSharedPointerDynamicCast<NodeVar>(node->m_left);
    QString str = as->NewLabel("stringassignstr");
    QString lblCpy=as->NewLabel("stringassigncpy");

    QString strAssign = str + "\tdb \"" + right->m_op.m_value + "\",0";
    // Temp vars are place with variables in the code, no need for a jmp
    as->m_tempVars<<strAssign;

    if (isPointer) {
        as->Asm("ld hl,"+str);
        as->Asm("ld ["+getValue(left)+"],hl");
    }
    else {
        // If not a pointer, copy everything
        as->Asm("ld hl,"+str);
        as->Asm("ld de,"+getValue(left));
        as->Label(lblCpy);
        if (isGB())
            as->Asm("ld a,[hl+]");
        else {
            as->Asm("ld a,[hl]");
            as->Asm("inc hl");

        }
        as->Asm("ld [de],a");
        as->Asm("inc de");
  //      as->Asm("dec c");
        as->Asm("cp 0");
        as->Asm("jr nz, "+lblCpy);
    }
    //  as->PopLabel("stringassign");
    as->PopLabel("stringassignstr");
    as->PopLabel("stringassigncpy");

}

QString ASTdispatcherZ80::getJmp(bool isOffPage) {
    if (!isOffPage)
        return "jr";
    return "jp";
}
/*
 *
 * Main method used in for loops. Will increase a counter in nodeA->m_left (from for a:=0 ...)
 * and then compare with the end value, jump if not equal
 */
void ASTdispatcherZ80::CompareAndJumpIfNotEqualAndIncrementCounter(QSharedPointer<Node> nodeA, QSharedPointer<Node> nodeB, QSharedPointer<Node> step, QString lblJump, bool isOffPage, bool isInclusive)
{

    QString var = nodeA->m_left->getValue(as);
  //  PopX();

    if (nodeA->m_left->isWord(as)) {
        if (!nodeB->isPure())
            ErrorHandler::e.Error("16 bit index counter target must be pure numeric or variable!",nodeB->m_op.m_lineNumber);

        QString ax = "hl";
        if (step!=nullptr) {
            as->Comment("; 16 bit counter with STEP");
            step->setForceType(TokenType::INTEGER);
            step->Accept(this);
            as->Comment(";here");
            as->Asm("ex de,hl");
            nodeA->m_left->Accept(this);
            as->Asm("add hl,de");

        }
        else {
            as->Comment("; 16 bit counter without STEP");
            nodeA->m_left->Accept(this);
            as->Term();
            as->Asm("inc "+ax);

        }
        as->Asm(m_mov+"["+var+"],"+ax);

        as->Asm("ld c,l");
        as->Asm("ld a,"+nodeB->getValue8bit(as,false));
        as->Asm(m_cmp+"c");

        if (!isOffPage)
            as->Asm("jr nz,"+lblJump);
        else
            as->Asm("jp nz,"+lblJump);

        as->Asm("ld c,h");
        as->Asm("ld a,"+nodeB->getValue8bit(as,true));
        as->Asm(m_cmp+"c");

        if (!isOffPage)
            as->Asm("jr nz,"+lblJump);
        else
            as->Asm("jp nz,"+lblJump);

        return;
    }

//    QString var = nodeA->m_left->getValue(as);

        if (!nodeB->isPureNumeric()) {
            as->ClearTerm();
            nodeB->Accept(this);
            as->Term();
            as->Asm("ld c,a");
        }

        // If step is a number, load it here
        // Post optimizer should take care of stuff
        if (step!=nullptr) {
            step->Accept(this);
            as->Asm("ld d,a");
        }
    //    PushX();
        QString ax = getAx(nodeA->m_left);
      //  PopX();
        as->Asm(m_mov+ax+",["+var+"]");

        if (step==nullptr)
            as->Asm("add "+ax+",1");
        else
            as->Asm("add "+ax+",d");

        as->Asm(m_mov+"["+var+"],"+ax);

        if (!nodeB->isPureNumeric())
            as->Asm(m_cmp+"c");
        else
            as->Asm(m_cmp+nodeB->getValue(as));


        if (!isOffPage)
            as->Asm("jr nz,"+lblJump);
        else
            as->Asm("jp nz,"+lblJump);
}

void ASTdispatcherZ80::CompareAndJumpIfNotEqual(QSharedPointer<Node> nodeA, QSharedPointer<Node> nodeB, QString lblJump, bool isOffPage)
{
    QString var = nodeA->getValue(as);

    if (!nodeB->isPureNumeric()) {
        as->ClearTerm();
        nodeB->Accept(this);
        as->Term();
        as->Asm("ld c,a");
    }


    nodeA->Accept(this);
    as->Term();

    if (!nodeB->isPureNumeric())
        as->Asm(m_cmp+"c");
    else
        as->Asm(m_cmp+nodeB->getValue(as));


    if (!isOffPage)
        as->Asm("jr nz,"+lblJump);
    else
        as->Asm("jp nz,"+lblJump);
}

/*
 * Handles a:=a op b  for 16 bit values
 *
 *
 */
void ASTdispatcherZ80::HandleAeqAopB16bit(QSharedPointer<NodeBinOP> bop, QSharedPointer<NodeVar> var)
{
    as->Comment("16 bit BINOP");
    /*
     * First case : is the RHS of the binary operation pure?
     */
    if (bop->m_right->isPure()) {
        as->Comment("RHS is pure ");
        // a := a + $200 etc
        if (bop->m_right->isPureNumeric() && (bop->m_right->getValueAsInt(as)&0xFF)==0) {
            as->Comment("RHS is pure constant of $100");
            as->Asm("ld b,"+Util::numToHex(bop->m_right->getValueAsInt(as)>>8));
            as->Asm("ld a,[ "+var->value+"+1]");
            as->Asm("add a,b");
            as->Asm("ld [ "+var->value+"+1],a");
            return;

        }
        m_useNext="de";
        bop->m_right->Accept(this);
    }
    else {
        bop->m_right->Accept(this);
        if (isGB()) {
            as->Asm("ld d,h");
            as->Asm("ld e,l");
        }
        else {
            as->Asm("ex de,hl");
        }
    }
    LoadVariable(var); // Load address into hl
    //    as->Comment(";Comment : "+node->m_left->isWord(as)+TokenType::getType(node->m_left->m_forceType));
        if (!var->isWord(as)) {
            as->Comment(";var is 8-bit, compensate");
            as->Asm("ld l,a");
            as->Asm("ld h,0");
        }
    // Plus is easy!
    if (bop->m_op.m_type==TokenType::PLUS) {
        as->Asm(getPlusMinus(bop->m_op)+" hl,de");
        StoreAddress(var);
        return;
    }

    as->Asm("ld a,l");
    as->Asm("or a");
    as->Asm("sub e");

    as->Asm("ld l,a");
    as->Asm("ld a,h");

    as->Asm("sbc a,d");
//    as->Asm("ld h,a");


    as->Asm("ld ["+var->value+"],a");
    as->Asm("ld a,l");
    as->Asm("ld ["+var->value+"+1],a");
//    StoreVariable(var);


}




void ASTdispatcherZ80::dispatch(QSharedPointer<NodeBinOP>node)
{
    if (node->m_left->isWord(as))
        node->m_right->setForceType(TokenType::INTEGER);

    if (node->m_right->isWord(as))
        node->m_left->setForceType(TokenType::INTEGER);


    if (node->isPureNumeric()) {
        as->Asm("ld "+getAx(node)+", " + node->getValue(as) + "; binop is pure numeric");
        return;
    }
    if (node->isPureVariable()) {
        as->Asm("ld "+getAx(node)+", [" + node->getValue(as)+"] ; binop is pure variable");
        return;
    }
    /*    if (!node->m_left->isPure() && node->m_right->isPure()) {
        QSharedPointer<Node> t = node->m_right;
        node->m_right = node->m_left;
        node->m_left = t;
        qDebug() << "SWITCH";
    }
*/
    if (node->m_op.m_type == TokenType::MUL || node->m_op.m_type == TokenType::DIV) {

        if (node->m_op.m_type == TokenType::DIV) {
            //            node->m_right->setForceType(TokenType::BYTE);
            ErrorHandler::e.Error("Generic division is not implemented yet!",  node->m_op.m_lineNumber);
            //            as->Asm("cwd");
        }
        if (node->m_op.m_type == TokenType::MUL) {
            QString skip1 = as->NewLabel("skip1");
            as->Comment("Generic mul");
            if (!node->isWord(as)) {
                node->m_right->Accept(this);
                as->Asm("ld e,a");
                as->Asm("ld d,0");
                node->m_left->Accept(this);
                as->Asm("ld h,a");
                as->Asm("ld l,0");
                as->Asm("call mul_8x8");
                as->Asm("ld a,l");

                return;
            }
            // Is 16 bit

            //                ErrorHandler::e.Error("NOT IMPLEMENTED YET", node->m_op.m_lineNumber);

                if (node->m_left->isPure()) {
                    m_useNext ="de";
                    node->m_left->Accept(this);
                }
                else {
                    node->m_left->Accept(this);
                    as->Asm("ld d,h");
                    as->Asm("ld e,l");
                }
                node->m_right->setForceType(TokenType::BYTE);
                node->m_right->Accept(this);
//                as->Asm("ld a,l");
                as->Asm("ld hl,0");
                as->Asm("ld c,0");
                as->Asm("call mul_16x8");
//                as->Asm("ld a,l");



//            }

//            ErrorHandler::e.Error("Generic 16-bit multiplication is not implemented yet!",  node->m_op.m_lineNumber);
              return;
        }
        node->m_left->Accept(this);
        QString bx = getAx(node->m_left);
        PushX();
        node->m_right->Accept(this);

        QString ax = getAx(node->m_right);
        PopX();
        as->BinOP(node->m_op.m_type);

        as->Asm(as->m_term + " " +  ax);
        as->m_term = "";
        return;
    }

    if (node->m_op.m_type == TokenType::SHR || node->m_op.m_type == TokenType::SHL) {
        if (node->getType(as)==TokenType::INTEGER) {
            Handle16bitShift(node);
            return;
        }


        if (!node->m_right->isPureNumeric()) {
            node->m_right->Accept(this);
            as->Asm("ld c,a");
            node->m_left->Accept(this);

            QString lbl = as->NewLabel("bitshift");
            as->Label(lbl);
            if (node->m_op.m_type == TokenType::SHR)
                as->Asm("rrca");
            if (node->m_op.m_type == TokenType::SHL)
                as->Asm("rlca");
            as->Asm("dec c");
            as->Asm("jr nz,"+lbl);
            as->PopLabel("bitshift");
            return;
        }
        node->m_left->Accept(this);

//            ErrorHandler::e.Error("bit shifting on the Z80 must have constant numeric values!", node->m_op.m_lineNumber);
        int num = node->m_right->getValueAsInt(as);
        if (node->m_op.m_type == TokenType::SHR)
            as->Asm("and "+Util::numToHex(0xFF^(int)(pow(2,num)-1)));
        for (int i=0;i<num;i++) {
            if (node->m_op.m_type == TokenType::SHR)
                as->Asm("rrca");
            if (node->m_op.m_type == TokenType::SHL)
                as->Asm("rlca");
        }
        return;
    }

    // Special case if B-node is simple:
    if (node->m_right->isPure() && node->m_left->isPure() && !node->m_right->isWord(as)) {
        if (node->m_right->isPureVariable())  {
            node->m_right->Accept(this);
            as->Asm("ld b,a");
        }
        else {
            as->Asm("ld b,"+node->m_right->getValue(as));
        }
        node->m_left->Accept(this);
        as->Asm(getBinaryOperation(node)+" b");
        return;
    }

    if (node->m_right->isWord(as)) {
        as->Comment("Generic 16-bit binop");
//        if (node->m_right->isWord(as))
  //          as->Asm("ld d,0");
        node->m_right->Accept(this);
        if (isGB()) {
            as->Asm("push hl");
            as->Asm("pop de");
        }
        else
            as->Asm("ex de,hl");

        node->m_left->Accept(this);
        as->Term();


        if (node->m_op.m_type == TokenType::PLUS) {
            as->Asm("add hl,de");
            return;
        }
        if (node->m_op.m_type == TokenType::MINUS) {
            as->Asm("sbc hl,de");
            return;

        }
        ErrorHandler::e.Error("TRSE currently does not support the following 16-bit binary opreation : "+node->m_op.getType(),node->m_op.m_lineNumber);

//        as->Asm(getBinaryOperation(node)+" hl,de");

        return;
    }

    node->m_left->Accept(this);
    as->Asm("push af");
    node->m_right->Accept(this);
    as->Asm("ld b,a");
    as->Asm("pop af");
    as->Asm(getBinaryOperation(node)+" b");

//    as->Asm("ld b,a");

/*    as->Comment(";balle binop");
    node->m_left->Accept(this);
    QString bx = getAx(node->m_left);

    PushX();
    node->m_right->Accept(this);

    QString ax = getAx(node->m_right);
    PopX();
    as->BinOP(node->m_op.m_type);

    as->Asm(as->m_term + " " +  bx +"," +ax);
    as->m_term = "";
*/



}



void ASTdispatcherZ80::dispatch(QSharedPointer<NodeBinaryClause> node)
{

}


/*
void ASTdispatcherZ80::dispatch(QSharedPointer<NodeConditional> node)
{

}
*/
/*
void ASTdispatcherZ80::dispatch(QSharedPointer<NodeForLoop> node)
{

}
*/
void ASTdispatcherZ80::dispatch(QSharedPointer<NodeVarDecl> node)
{
    QSharedPointer<NodeVar> v = qSharedPointerDynamicCast<NodeVar>(node->m_varNode);
    QSharedPointer<NodeVarType> t = qSharedPointerDynamicCast<NodeVarType>(node->m_typeNode);

    QSharedPointer<Appendix> old = as->m_currentBlock;

    if (t->m_flags.contains("hram")) {
        as->m_currentBlock = as->m_hram;
    }
    if (t->m_flags.contains("wram")) {
        as->m_currentBlock = as->m_wram;
    }

    if (t->m_flags.contains("sprram"))
        as->m_currentBlock = as->m_sprram;

    if (t->m_flags.contains("ram"))
        as->m_currentBlock = as->m_ram;

    if (t->m_flags.contains("bank")) {
        QString bnk = t->m_flags[t->m_flags.indexOf("bank")+1];//Banks always placed +1
        if (!as->m_banks.contains(bnk)) {
            as->m_banks[bnk] = QSharedPointer<Appendix>(new Appendix());
            as->m_banks[bnk]->m_pos = "$4000";
            as->m_banks[bnk]->m_isMainBlock = true;
        }
        as->m_currentBlock = as->m_banks[bnk];
    }
    if (v->m_isGlobal) {
        as->m_currentBlock = nullptr;
        return;
    }


//    qDebug() << "" <<as->m_currentBlock;
    AbstractASTDispatcher::dispatch(node);
  //  qDebug() << as->m_currentBlock;
    as->m_currentBlock = old;
}

void ASTdispatcherZ80::dispatch(QSharedPointer<NodeVar> node)
{

    if (m_inlineParameters.contains(node->value)) {
  //      qDebug()<< "INLINE node override : "<< node->value;
        m_inlineParameters[node->value]->Accept(this);
        return;
    }

    if (node->m_expr!=nullptr) {
        if (node->m_expr->isPureNumeric() && node->m_expr->getValueAsInt(as)==0) {
            as->Asm("; Optimization : zp[0]");
//            LoadVariable(as,)
//            as->Asm("ld hl,"+node->getValue(as));
            LoadAddress(node);
            as->Asm("ld a,[hl]");
            return;
        }
        if (node->m_expr->isPureNumeric()) {
            as->Asm("ld de,"+node->m_expr->getValue(as));
        }
        else {
        node->m_expr->Accept(this);
    /*        if (node->isWord(as))
                as->Asm("rlca"); // *2 for integer arrays
      */
            as->Term();
            as->Asm("ld e,a ; variable is 8-bit");
            as->Asm("ld d,0");
        }
  //      as->Asm("ld hl,"+node->getValue(as));
        LoadAddress(node);
        as->Asm("add hl,de");
        if (node->isWord(as))
            as->Asm("add hl,de");



        as->Asm("ld a,[hl]");

        as->Comment("LoadVar Testing if '"+node->getValue(as)+"' is word : "+QString::number(node->isWord(as)));
        if (node->isWord(as)) // More complicated: Load integer byte array into de
        {
                as->Asm("ld e,a");
                as->Asm("inc hl");
                as->Asm("ld a,[hl]");
                as->Asm("ld d,a");
                as->Asm("ex de,hl");
        }

        return;



/*        as->Asm("mov ah,0");
        node->m_expr->Accept(this);
        as->Asm("mov di,ax");
        if (node->getArrayType(as)==TokenType::INTEGER)
            as->Asm("shl di,1 ; Accomodate for word");
        ending = "+di]";*/

    }

    if (!node->isWord(as)) {
//        as->Comment("HERE") ;
        if (node->isPureVariable())
            as->Asm("ld a,["+node->getValue(as)+"]");
        else
            as->Asm("ld a,"+node->getValue(as));

  /*      if (node->m_forceType==TokenType::INTEGER) {
            as->Comment("; Byte, but forced to be integer");
        }
*/
        return;
    }
    // 16 bit all
    // Word
    as->Comment("Variable is 16-bit");

    if (node->isPureNumeric()) {
        QString hl =getHL();

        as->Asm("ld "+hl+", "+node->getValue(as));
    }
    else
    {

        if (node->getOrgType(as)==TokenType::POINTER) {
            //as->Asm("ld a,["+node->getValue(as)+"+1]");
            //as->Asm("ld "+QString(hl[0])+",a");
            LoadAddress(node);
        }
        else if (node->getOrgType(as)==TokenType::INTEGER) {
            //LoadVariable(node);
            //node->Accept(this);
            as->Comment("Integer");
            LoadInteger(node);

        }
        else {
            QString hl =getHL();

            if (node->isReference()) {
//                as->Comment("")
                as->Asm("ld "+hl+","+node->getValue(as));
                return;

            }

            as->Asm("ld a,["+node->getValue(as)+"]");
            as->Asm("ld "+QString(hl[1])+",a");
            if (Syntax::s.m_currentSystem->m_system==AbstractSystem::GAMEBOY) {
//                as->Asm("xor a,a");
                as->Asm("ld "+QString(hl[0])+",0");
            }
            else
            {
//                as->Asm("xor a");
  //          as->Asm("ld "+QString(hl[0])+",a");
                as->Asm("ld "+QString(hl[0])+",0");
            }
        }

    }
}

void ASTdispatcherZ80::dispatch(QSharedPointer<NodeNumber> node)
{
    QString hl = getHL();

    if (!node->isWord(as))
        as->Asm("ld a,"+node->getValue(as));
    else
        as->Asm("ld "+hl+","+node->getValue(as));


}

/*void ASTdispatcherZ80::dispatch(QSharedPointer<NodeForLoop> node)
{
    node->DispatchConstructor(as,this);


    //QString m_currentVar = ((NodeAssign*)m_a)->m_
    QSharedPointer<NodeAssign> nVar = qSharedPointerDynamicCast<NodeAssign>(node->m_a);


    if (nVar==nullptr)
        ErrorHandler::e.Error("Index must be variable", node->m_op.m_lineNumber);

    QString var = qSharedPointerDynamicCast<NodeVar>(nVar->m_left)->getValue(as);//  m_a->Build(as);
    //    qDebug() << "Starting for";
    node->m_a->Accept(this);
    //  qDebug() << "accepted";

    //    LoadVariable(node->m_a);
    //  TransformVariable()
    //QString to = m_b->Build(as);
    QString to = "";
    if (node->m_b->isPure())
        //    if (qSharedPointerDynamicCast<NodeNumber>(node->m_b) != nullptr)
        to = node->m_b->getValue(as);
    //  if (qSharedPointerDynamicCast<NodeVar>(node->m_b) != nullptr)
    //    to = (qSharedPointerDynamicCast<NodeVar>node->m_b)->getValue(as);

    //    as->m_stack["for"].push(var);
    QString lblFor =as->NewLabel("forloop");
    as->Label(lblFor);
    //    qDebug() << "end for";


    if (nVar->m_left->isWord(as))
        node->m_b->setForceType(TokenType::INTEGER);

    node->m_block->Accept(this);
    QString ax = getA(nVar->m_left);
    if (!node->m_b->isPureNumeric()) {
        as->ClearTerm();
        node->m_b->Accept(this);
        as->Term();
        as->Asm("ld c,a");
    }

    as->Asm(m_mov+ax+",["+var+"]");
    as->Asm("add "+ax+",1");
    as->Asm(m_mov+"["+var+"],"+ax);
    if (!node->m_b->isPureNumeric())
        as->Asm(m_cmp+ax+",c");
    else
        as->Asm(m_cmp+ax+"," + node->m_b->getValue(as));

    bool offpage = node->m_forcePage;
    if (!node->verifyBlockBranchSize(as, node->m_block, nullptr, this))
        offpage = true;

    if (!offpage)
        as->Asm("jr nz,"+lblFor);
    else
        as->Asm("jp nz,"+lblFor);

    as->PopLabel("forloop");

}

*/

QString ASTdispatcherZ80::AssignVariable(QSharedPointer<NodeAssign> node)
{

    if (node->m_left->isWord(as)) {
        node->m_right->setForceType(TokenType::INTEGER);
    }
    as->ClearTerm();

    if (qSharedPointerDynamicCast<NodeString>(node->m_right))
    {
        AssignString(node,node->m_left->isPointer(as));
        return "";
    }


    QSharedPointer<NodeVar> var = qSharedPointerDynamicCast<NodeVar>(node->m_left);
    as->ClearTerm();

    if (qSharedPointerDynamicCast<NodeNumber>(node->m_left)!=nullptr) {
        node->m_right->Accept(this);
        as->Term();
        as->Asm("ld ["+node->m_left->getValue(as)+"],a");
        return "";

    }


    if (var->isPointer(as)) {
        HandleAssignPointers(node);
        return "";
    }

    if (var->isArrayIndex() && !var->isWord(as)) {
        // Is an array

        // Optimization : array[ constant ] := expr
        if (var->m_expr->isPureNumeric()) {
            node->m_right->Accept(this);
            as->Term();

            as->Asm("ld [+"+var->getValue(as)+"+"+var->m_expr->getValue(as)+"],a");
            return "";
        }
        // GENERIC expression

        bool rightIsPure = node->m_right->isPure();
        bool isAligned = as->m_symTab->Lookup(var->getValue(as),var->m_op.m_lineNumber)->m_flags.contains("aligned");
//        qDebug() << "IS ALIGNED "<< isAligned;

        if (!rightIsPure) {
            node->m_right->Accept(this);
            as->Asm("push af");
        }
        var->m_expr->Accept(this);
        if (isAligned) {
            as->Asm("ld hl," + var->getValue(as));
            as->Asm("add a,l");
            as->Asm("ld l,a");
        }
        else {
            as->Asm("ld e,a");
            as->Asm("ld d,0");
            as->Asm("ld hl," + var->getValue(as));
            as->Asm("add hl,de");
        }
        if (!rightIsPure)
            as->Asm("pop af");
        else
            node->m_right->Accept(this);

        as->Asm("ld [hl],a");
        return "";


//        ErrorHandler::e.Error("Array indicies not implemented yet");


  /*      node->m_right->Accept(this);
        as->Asm("push a");
        var->m_expr->Accept(this);
        as->Asm("ld d,a");
        if (var->isWord(as))
            as->Asm("shl d,1");
        as->Asm("pop a");
        as->Asm("ld ["+var->getValue(as) + "+di], "+getAx(node->m_left));
        return "";*/
    }

    //    if (var->getValue())
    // Simple a:=b;
    QString type ="byte";
    if (var->isWord(as))
        type = "word";

    if (node->m_right->isPureNumeric() && !var->isWord(as)) {
        as->Asm("ld "+getAx(node) + ", "+node->m_right->getValue(as));
        as->Asm("ld ["+var->getValue(as)+ "], "+getAx(node));
        return "";
    }
    // Check for a:=a+2;
    QSharedPointer<NodeBinOP> bop =  qSharedPointerDynamicCast<NodeBinOP>(node->m_right);
    // as->Comment("Testing for : a:=a+ expr " + QString::number(bop!=nullptr));
    // if (bop!=nullptr)
    //  as->Comment(TokenType::getType(bop->getType(as)));
    if (bop!=nullptr && (bop->m_op.m_type==TokenType::PLUS || bop->m_op.m_type==TokenType::MINUS)) {
        //      as->Comment("PREBOP searching for "+var->getValue(as));
        if (bop->ContainsVariable(as,var->getValue(as))) {
            // We are sure that a:=a ....
            // first, check if a:=a + number
            //            as->Comment("In BOP");

            // Handle 16 bit op
            if (bop->isWord(as)) {
                HandleAeqAopB16bit(bop,var);
                return "";
            }


            if (bop->m_right->isPureNumeric()) {
                as->Comment("'a:=a + const'  optimization ");
                //as->Asm(getBinaryOperation(bop) + " ["+var->getValue(as)+"], "+type + " "+bop->m_right->getValue(as));
                as->Asm("ld a,["+var->getValue(as)+"]");
                as->Asm(getBinaryOperation(bop)  +bop->m_right->getValue(as));
                as->Asm("ld ["+var->getValue(as)+"], a");
                return "";
            }
            as->Comment("'a:=a + expression'  optimization ");
            bop->m_right->Accept(this);
            as->Asm("ld b,a");
            as->Asm("ld a,["+var->getValue(as)+"]");
            as->Asm(getBinaryOperation(bop) + " b");
            as->Asm("ld ["+var->getValue(as)+"], a");

            return "";
        }
        // Check for a:=a+

    }
    /*    if (node->m_right->isPureVariable()) {
            as->Asm("mov ["+var->getValue(as)+ "],   " +getX86Value(as,node->m_right));
            return "";
        }
    */
    as->Comment("generic assign ");
    QString name = var->getValue(as);
    if (var->isWord(as)) {
        node->m_right->setForceType(TokenType::INTEGER);

        node->m_right->Accept(this);
        if (var->isArrayIndex())  { // Storing in 16 bit array index

            as->Comment("Storing in 16-bit array index");
            as->Asm("push hl");
            LoadAddress(var);
            var->m_expr->Accept(this);
            as->Asm("ld d,0");
            as->Asm("ld e,a");
            as->Asm("add hl,de");
            as->Asm("add hl,de");
            as->Asm("pop de");
            as->Asm("ld a,e");
            as->Asm("ld [hl],a");
            as->Asm("ld a,d");
            as->Asm("inc hl");
            as->Asm("ld [hl],a");
            return "";
        }


        as->Comment("Integer assignment ");
        if (!isGB())
            as->Asm("ld ["+name+"],hl");
        else {
            as->Asm("ld a,h");
            as->Asm("ld ["+name+"+1],a");
            as->Asm("ld a,l");
            as->Asm("ld ["+name+"],a");
        }
        return "";
    }
    node->m_right->Accept(this);


    if (node->m_left->isArrayIndex()) {
        as->Asm("ld [hl], a");
        return "";
    }
    if (node->m_right->isWord(as) && !node->m_left->isWord(as)) {
        as->Asm("ld l,a ; word assigned to byte");
    }

    as->Asm("ld ["+name + "], "+getAx(node->m_left));
    return "";
}

QString ASTdispatcherZ80::getAx(QSharedPointer<Node> n) {
    QString a = m_regs[m_lvl];
    if (n->m_forceType==TokenType::INTEGER)
        return "bc";
    if (n->getType(as)==TokenType::INTEGER)
        return "bc";
    if (n->getType(as)==TokenType::ADDRESS)
        return "hl";
    if (n->getType(as)==TokenType::INTEGER_CONST)
        if (n->isWord(as))
            return "bc";
    //        if (n->isPureNumeric())
    //          if (n->getValue()
    return a;

}

QString ASTdispatcherZ80::getA(QSharedPointer<Node> n) {
    QString a = m_regs[m_lvl];
    return a;

}

QString ASTdispatcherZ80::getX86Value(Assembler *as, QSharedPointer<Node> n) {
    if (n->isPureVariable())
        return "["+n->getValue(as)+"]";
    return n->getValue(as);

}

QString ASTdispatcherZ80::getBinaryOperation(QSharedPointer<NodeBinOP> bop) {

    QString add = "";
//    if (Syntax::s.m_currentSystem->m_processor==AbstractSystem::GBZ80)
        add = " a,";

    if (bop->m_op.m_type == TokenType::PLUS)
        return "add "+add;
    if (bop->m_op.m_type == TokenType::MINUS)
        return "sub ";//+add;
    if (bop->m_op.m_type == TokenType::BITAND)
        return "and ";
    if (bop->m_op.m_type == TokenType::BITOR)
        return "or ";
    if (bop->m_op.m_type == TokenType::XOR)
        return "xor ";
/*    if (bop->m_op.m_type == TokenType::DIV)
        return "div";
    if (bop->m_op.m_type == TokenType::MUL)
        return "mul";*/
    ErrorHandler::e.Error("Unknown binary operation : " + bop->m_op.getType(),bop->m_op.m_lineNumber);
    return "UNKNOWN BINARY OPERATION";
}

void ASTdispatcherZ80::LoadAddress(QSharedPointer<Node> n)
{
    QString hl =getHL();
    if (n->isPointer(as)) {
        if (isGB()) {
            as->Asm("ld a,[" +n->getValue(as)+"]");
            as->Asm("ld "+QString(hl[0])+",a");
            as->Asm("ld a,[" +n->getValue(as)+"+1]");
            as->Asm("ld "+QString(hl[1])+",a");


        }
        else
//            if (n->isReference())
  //              as->Asm("ld "+hl+","+n->getValue(as)+"");
    //    else
            as->Asm("ld "+hl+",["+n->getValue(as)+"]");

    }
    else as->Asm("ld "+hl+"," +n->getValue(as));
}

void ASTdispatcherZ80::LoadInteger(QSharedPointer<Node> n)
{
    QString hl =getHL();

    if (isGB()) {
        as->Asm("ld a,[" +n->getValue(as)+"]");
        as->Asm("ld "+QString(hl[0])+",a");
        as->Asm("ld a,[" +n->getValue(as)+"+1]");
        as->Asm("ld "+QString(hl[1])+",a");
    }
    else
    as->Asm("ld "+hl+",["+n->getValue(as)+"]");
}


void ASTdispatcherZ80::StoreAddress(QSharedPointer<Node> n)
{
    QString hl =getHL();
    if (!n->isWord(as)) {
        as->Comment("Stored address is byte, not var: compensate");
        as->Asm("ld a,l");
        as->Asm("ld ["+n->getValue(as)+"],a");
        return;
    }
    if ( isGB()) {
        as->Asm("ld a,"+QString(hl[0])+"");
        as->Asm("ld ["+n->getValue(as)+"], a");
        as->Asm("ld a,"+QString(hl[1])+"");
        as->Asm("ld ["+n->getValue(as)+"+1], a");

    }
    else
        as->Asm("ld ["+n->getValue(as)+"],"+hl);
}

QString ASTdispatcherZ80::getHL() {
    QString hl ="hl";
    if (m_useNext!="")
        hl = m_useNext;
    m_useNext="";
    return hl;
}

void ASTdispatcherZ80::BuildSimple(QSharedPointer<Node> node,  QString lblSuccess, QString lblFailed, bool offPage)
{

    as->Comment("Binary clause core: " + node->m_op.getType());
    //    as->Asm("pha"); // Push that baby

    if (node->m_op.m_type==TokenType::LESSEQUAL) {
        auto n = node->m_right;
        node->m_right = node->m_left;
        node->m_left = n;
    }

    BuildToCmp(node);


    QSharedPointer<NodeConditional> n = qSharedPointerDynamicCast<NodeConditional>(node);
    QString p = "r";
    if (offPage)
        p="p";

    if (node->m_op.m_type==TokenType::EQUALS)
        as->Asm("j"+p+" nz," + lblFailed);
    if (node->m_op.m_type==TokenType::NOTEQUALS)
        as->Asm("j"+p+" z, " + lblFailed);
    if (node->m_op.m_type==TokenType::LESS) {
        as->Asm("j"+p+" nc," + lblFailed);
//        as->Asm("j"+p+" z, " + lblFailed);
    }
    if (node->m_op.m_type==TokenType::GREATER) {
        as->Asm("j"+p+" c, " + lblFailed);
        as->Asm("j"+p+" z, " + lblFailed);
    }
    if (node->m_op.m_type==TokenType::GREATEREQUAL) {
        as->Asm("j"+p+" c, " + lblFailed);

    }
    if (node->m_op.m_type==TokenType::LESSEQUAL) {
//        as->Asm("j"+p+" z, " + lblFailed);
        // Flipped!
        as->Asm("j"+p+" c," + lblFailed);
    }



}

void ASTdispatcherZ80::BuildToCmp(QSharedPointer<Node> node)
{

    if (node->m_left->getValue(as)!="") {
        if (node->m_right->isPureNumeric())
        {
            as->Comment("Compare with pure num / var optimization");
            //            TransformVariable(as,"cmp",node->m_left->getValue(as),node->m_right->getValue(as),node->m_left);
            //            TransformVariable(as,"cmp",node->m_left->getValue(as),node->m_right->getValue(as),node->m_left);
            LoadVariable(node->m_left);
            as->Asm("cp " + node->m_right->getValue(as));

            return;
        } else
        {
            as->Comment("Compare two vars optimization");
            if (node->m_right->isPureVariable()) {
                //QString wtf = as->m_regAcc.Get();
                LoadVariable(node->m_right);
                as->Asm("ld b,a");
                LoadVariable(node->m_left);
                //TransformVariable(as,"move",wtf,qSharedPointerDynamicCast<NodeVar>node->m_left);
                //TransformVariable(as,"cmp",wtf,as->m_varStack.pop());
                as->Asm("cp b");

                return;
            }
/*
            node->m_right->Accept(this);

            as->Asm("cp " + getAx(node->m_right));

            //            TransformVariable(as,"cmp",qSharedPointerDynamicCast<NodeVar>node->m_left,as->m_varStack.pop());
            return;*/
        }
    }
    node->m_right->Accept(this);
    as->Term();
    as->Asm("ld c, a");
    node->m_left->Accept(this);
    as->Term();

    as->Asm("cp c");

    //    TransformVariable(as,"cmp",qSharedPointerDynamicCast<NodeVar>node->m_left, as->m_varStack.pop());

    // Perform a full compare : create a temp variable
    //        QString tmpVar = as->m_regAcc.Get();//as->StoreInTempVar("binary_clause_temp");
    //        node->m_right->Accept(this);
    //      as->Asm("cmp " + tmpVar);
    //      as->PopTempVar();


}

void ASTdispatcherZ80::HandleAssignPointers(QSharedPointer<NodeAssign> node)
{
    //        qDebug() << "IS POINTER "<<node->m_right->isPure();
    QSharedPointer<NodeVar> var = qSharedPointerDynamicCast<NodeVar>(node->m_left);
    node->VerifyReferences(as);

    if (!var->isArrayIndex()) {

        // P := Address / variable
        if (node->m_right->isPure()) {
            LoadAddress(node->m_right);
            StoreAddress(var);
            return;
        }
        // First test for the simplest case: p := p  + const
        QSharedPointer<NodeBinOP> bop = qSharedPointerDynamicCast<NodeBinOP>(node->m_right);
        // P:= (expr);
        if (bop!=nullptr) {
            // Pointer := Pointer BOP expr
            if (bop->m_left->getValue(as)==var->getValue(as)) {
                as->Comment(";generic pointer/integer P:=P+(expr) add expression");

                bop->setForceType(TokenType::INTEGER);
                if (bop->m_right->isPure()) {
                    as->Comment("RHS is pure ");
                    if (bop->m_right->isPureNumeric() && (bop->m_right->getValueAsInt(as)&0xFF)==0) {
                        as->Comment("RHS is pure constant of $100");
                        if (isGB()) {
                        as->Asm("ld b,"+Util::numToHex(bop->m_right->getValueAsInt(as)>>8));
                        as->Asm("ld a,[ "+var->value+"]");
                        as->Asm("add a,b");
                        as->Asm("ld [ "+var->value+"],a");
                        }
                        else {
                            as->Asm("ld b,"+Util::numToHex(bop->m_right->getValueAsInt(as)>>8));
                            as->Asm("ld a,[ "+var->value+"+1]");
                            as->Asm("add a,b");
                            as->Asm("ld [ "+var->value+"+1],a");

                        }
                        return;

                    }
                    m_useNext="de";
                    bop->m_right->Accept(this);
                }
                else {
                    as->Comment("Executing 16 bit binop here");
                    bop->Accept(this);
/*                    as->Asm("ld d,h");
                    as->Asm("ld e,l");*/

/*                    as->Asm("ld e,a");
                    as->Asm("inc hl");
                    as->Asm("ld a,[hl]");
                    as->Asm("ld d,a");
                    as->Asm("ex de,hl");*/
                    StoreAddress(var);
                    return;

                }
                LoadAddress(var); // Load address into hl
                if (bop->m_op.m_type==TokenType::PLUS) {
                    as->Asm(getPlusMinus(bop->m_op)+" hl,de");
                    StoreAddress(var);
                    return;
                }

                as->Asm("ld a,l");
                as->Asm("or a");
                as->Asm("sub e");

                as->Asm("ld l,a");
                as->Asm("ld a,h");

                as->Asm("sbc a,d");
                as->Asm("ld h,a");


                StoreAddress(var);


                // DAMN. Is minus.
                /*
                ld a,


                  */
                //                as->Asm(getPlusMinus(bop->m_op)+" hl,de");
                // Store address

                return;
            }


//            else ErrorHandler::e.Error("Pointers in GBZ80 TRSE do not support this expression",bop->m_op.m_lineNumber);
        }
        // Generic : Doesn't really work
        as->Comment("Generic assign 16-bit pointer");
        node->m_right->setForceType(TokenType::INTEGER);
        node->m_right->Accept(this);
        as->Comment("Store 16-bit address");
        StoreAddress(var);
        return;

        // Generic case ))
    }
    else
    {
        // HAS array index, so P[ index ] := ..
        // Much simpler. set value.
        if (!node->m_right->isPure()) {
            node->m_right->Accept(this);
            as->Asm("push af");
        }
        LoadAddress(var);
        /*        as->Asm("ld a,[p1]");
        as->Asm("ld h,a");
        as->Asm("ld a,[p1+1]");
        as->Asm("ld l,a");*/
        if (var->m_expr->isPureNumeric() && var->m_expr->getValueAsInt(as)==0) {

        }
        else
        {
            if (var->m_expr->isPureNumeric()) {
                as->Comment("; index is pure number optimization");
                as->Asm("ld de,"+var->m_expr->getValue(as));
                as->Asm("add hl,de");
            }
            else {
                if (var->m_expr->isWord(as)) {
                    as->Comment(";general 16-bit expression for the index");
                    as->Asm("ex de,hl ;keep");
                    var->m_expr->Accept(this);
                    as->Asm("ex de,hl");
                    as->Asm("add hl,de");
                }
                else {
                    as->Comment(";general 8-bit expression for the index");
    //                as->Asm("xor a");
                    as->Asm("ld d,0");
                    var->m_expr->Accept(this);
                    as->Asm("ld e,a");
                    as->Asm("add hl,de");
                }
            }
        }
        if (!node->m_right->isPure()) {
            as->Asm("pop af");
        }
        else
            if (node->m_right->isWord(as)) {
                if (node->m_right->isPure()) {
                    as->Comment("Optimization: rhs is integer, but pure");
                    if (node->m_right->isPureVariable())
                        as->Asm("ld a,["+node->m_right->getValue(as)+"]");
                    else // is number
                        as->Asm("ld a,"+Util::numToHex(node->m_right->getValueAsInt(as)&0xFF));

                }
                else {
                    as->Asm("push hl");
                    node->m_right->Accept(this);
                    as->Term();
                    as->Asm("ld a,l");
                    as->Asm("pop hl");
                }
            }
            else
                node->m_right->Accept(this);
        as->Asm("ld [hl],a"); // Store in pointer

        return;
    }
    ErrorHandler::e.Error("Pointers must be assigned to variables or addresses.",node->m_op.m_lineNumber);

}

QString ASTdispatcherZ80::getPlusMinus(Token t) {
    if (t.m_type == TokenType::PLUS) {
        return "add ";
    }
    if (t.m_type == TokenType::MINUS) {
        return "sub ";
    }
    ErrorHandler::e.Error("Only plus / minus are supported for this type of operation.",t.m_lineNumber);
    return "";
}

