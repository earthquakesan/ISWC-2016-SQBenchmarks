//
// C++ Implementation: Generator 
//
// Author: Michael Schmidt, Christoph Pinkel <>, (C) 2007-2008
// Copyright: See COPYING file that comes with this distribution
//
#include "generator.h"

/*!
    Constructor
*/
Generator::Generator(CmdLine* _cmd)
    : cmd(_cmd), olm(_cmd), auths(new ActiveAuthors()), missingcites()
{
}

Generator::~Generator()
{
    auths->retire(auths->count());
    delete auths;
}

/*!
    \fn Generator::generate(unsigned)
    Generates all information and stores it on the disk
 */
void Generator::generate()
{
    //creating/cleaning output file, filling with prefixes...
    FILE *f = fopen(cmd->getOutFileName(), "w+");
    fputs("@prefix dc: <http://purl.org/dc/elements/1.1/> .\n", f);
    fputs("@prefix dcterms: <http://purl.org/dc/terms/> .\n", f);
    fputs("@prefix rdf:  <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n", f);
    fputs("@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n", f);
    fputs("@prefix swrc: <http://swrc.ontoware.org/ontology#> .\n", f);
    fputs("@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n", f);
    fputs("@prefix bench: <http://localhost/vocabulary/bench/> .\n", f);
    fputs("@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n", f);
    fputs("@prefix person: <http://localhost/persons/> .\n", f);
    fputs("bench:Journal rdfs:subClassOf foaf:Document.\n", f);
    fputs("bench:Proceedings rdfs:subClassOf foaf:Document.\n", f);
    fputs("bench:Inproceedings rdfs:subClassOf foaf:Document.\n", f);
    fputs("bench:Article rdfs:subClassOf foaf:Document.\n", f);
    fputs("bench:Www rdfs:subClassOf foaf:Document.\n", f);
    fputs("bench:MastersThesis rdfs:subClassOf foaf:Document.\n", f);
    fputs("bench:PhDThesis rdfs:subClassOf foaf:Document.\n", f);
    fputs("bench:Incollection rdfs:subClassOf foaf:Document.\n", f);
    fputs("bench:Book rdfs:subClassOf foaf:Document.\n", f);
    fputs("<http://localhost/persons/Paul_Erdoes> rdf:type foaf:Person.\n",f);
    fputs("<http://localhost/persons/Paul_Erdoes> foaf:name \"Paul Erdoes\"^^xsd:string.\n",f);
    fputs("<http://localhost/misc/UnknownDocument> rdf:type foaf:Document.\n",f);
    fclose(f);

    olm.add(12, 1087); //register nr triples + prefix & def. triple byte size

    //computing output year by year, starting at DBLP_STARTYEAR 
    unsigned long year = DBLP_STARTYEAR;
    
    unsigned long int total_triples=0;
    unsigned long int total_kb=0;

    // keep track of total number of document class instances
    unsigned total_nr_inproceedings=0;
    unsigned total_nr_proceedings=0;
    unsigned total_nr_articles=0;
    unsigned total_nr_journals=0;
    unsigned total_nr_incollections=0;
    unsigned total_nr_books=0;
    unsigned total_nr_wwws=0;
    unsigned total_nr_masterstheses=0;
    unsigned total_nr_phdtheses=0;
    unsigned total_incoming_cites=0; // incoming citations up to now

    while(! olm.outputComplete())
    {
        cout << "### Calculating document contents for year "
            << year << "..." << endl;
        generateYear(year,total_nr_inproceedings,total_nr_proceedings,
                        total_nr_articles,total_nr_journals,
                        total_nr_incollections,total_nr_books,
                        total_nr_wwws,total_nr_masterstheses,
                        total_nr_phdtheses,total_incoming_cites);

        // calculate and print some statistics for the current year  
        unsigned long int new_total_triples=olm.getTotalTriples();
        unsigned long int new_total_kb=olm.getTotalKB();
     
        cout << "[" << year << "] Triples in year="
                << new_total_triples-total_triples
                << " ---> total triples=" << new_total_triples << endl;
        cout << "[" << year << "] Data written in year="
                << new_total_kb-total_kb << "KB"
                << " ---> total file size=" << new_total_kb << "KB" << endl;

        #ifdef LOGMODE
        cout << "-----------------------------------------------------------" << endl;
        cout << "[" << year << "] total nr inproceedings up to date="
            << total_nr_inproceedings << endl;
        cout << "[" << year << "] total nr proceedings up to date="
                << total_nr_proceedings << endl;
        cout << "[" << year << "] total nr articles up to date="
                << total_nr_articles << endl;
        cout << "[" << year << "] total nr journals up to date="
                << total_nr_journals << endl;
        cout << "[" << year << "] total nr incollections up to date="
                << total_nr_incollections << endl;
        cout << "[" << year << "] total nr books up to date="
                << total_nr_books << endl;
        cout << "[" << year << "] total nr wwws up to date="
                << total_nr_wwws << endl;
        cout << "[" << year << "] total nr masterstheses up to date="
                << total_nr_masterstheses << endl;
        cout << "[" << year << "] total nr phdtheses up to date="
                << total_nr_phdtheses << endl;
        cout << "[" << year << "] total incoming cites up to next year="
                << total_incoming_cites << endl;
        #endif // LOGMODE
        cout << endl;

		// store values for current year 
        total_triples=new_total_triples;
        total_kb=new_total_kb;

        year++;
    } //while

} //generate()


/*!
    \fn Generator::generateYear(unsigned)
    Generates new library information/new entries for specified year
 */
void Generator::generateYear(unsigned year,
                                unsigned &total_nr_inproceedings,
                                unsigned &total_nr_proceedings,
                                unsigned &total_nr_articles,
                                unsigned &total_nr_journals,
                                unsigned &total_nr_incollections,
                                unsigned &total_nr_books,
                                unsigned &total_nr_wwws,
                                unsigned &total_nr_masterstheses,
                                unsigned &total_nr_phdtheses,
								unsigned &total_incoming_cites)
{

char *powerauthor_uri = "<http://localhost/persons/Paul_Erdoes>";
unsigned pa_yr_cnt = 0; //power author paper count; power author doesn't care for
    //co author relations, triple or size constraints and publishes 10 papers every year

    //conferences, journals
    RDFTriplePool c_pool = generateConferences(year,total_nr_proceedings);
    RDFTriplePool j_pool = generateJournals(year,total_nr_journals);

    //books, incoll, phd, masters, www
    RDFTriplePool other_pool = generateOthers(year,total_nr_incollections,
                                                total_nr_books,total_nr_wwws,
                                                total_nr_masterstheses,total_nr_phdtheses);
    
    //talks, articles, also considering their relations to conferences, journals
    RDFTriplePool t_pool = generateInProceedings(year, &c_pool,total_nr_inproceedings);
    RDFTriplePool a_pool = generateArticles(year, &j_pool,total_nr_articles);
    
    RDFTriplePool garbage_pool = RDFTriplePool();

    //populating garbage collection:
    garbage_pool.addPool(&j_pool);
    garbage_pool.addPool(&other_pool);
    garbage_pool.addPool(&t_pool);
    garbage_pool.addPool(&a_pool);

    //authors
    //estimation of author numbers/counts...
    unsigned anum_distinct = Functions::nrDistinctAuthors(year);
    unsigned anum_returning = anum_distinct - Functions::nrNewAuthors(year);
    unsigned anum_new = Functions::nrNewAuthors(year);

    //pooling all publications that might require an author
    RDFTriplePool publication_pool;
    publication_pool.addPool(&t_pool);
    publication_pool.addPool(&a_pool);
    publication_pool.addPool(&other_pool);

    //pooling authors to work with...
    vector<Author *> picked_authors = auths->draw(anum_returning);

    auths->insertNew(anum_new, picked_authors, year);

    //getting numbers of authors, with i publications in this year each
    vector <unsigned> authors_with_publs;

    double param=Functions::fitAuthorsWithNumPublicationsParameter(year);
    for(unsigned i = 1; Functions::authorsWithNumPublications(i,year,param) > 0; i++)
        authors_with_publs.push_back(Functions::authorsWithNumPublications(i,year,param));

    #ifdef LOGMODE
    vector <unsigned> authors_with_publs_copy = authors_with_publs;
    #endif

    //assigning properties number of publ count, co-author count, distinct co-author count
    //(within this year) to authors:
    unsigned pcnt = authors_with_publs.size() - 1; //starting to assign max publications
    for(unsigned i = 0; i < picked_authors.size(); i++)
    {
        //number of publications:
        if(authors_with_publs[pcnt] == 0)
            pcnt--;

        picked_authors[i]->assignPublExp(pcnt+1);
        authors_with_publs[pcnt]--;
            
        //expectation value for co-authors (for particular author up to this year)
        double mu_coauth = Functions::muCoauth(picked_authors[i]->publications()
            + picked_authors[i]->expectedPublications());
        double mu_dist_coauth = Functions::muDistCoauth((unsigned)mu_coauth);
        
        int coauth_delta = (unsigned)mu_coauth - picked_authors[i]->coAuthors();
        int dist_coauth_delta = (unsigned)mu_dist_coauth
            - picked_authors[i]->distinctCoAuthors();
    
        if(coauth_delta < 0) coauth_delta = 0; //just in case, should not happen
        if(dist_coauth_delta < 0) dist_coauth_delta = 0;
        
        if(dist_coauth_delta > coauth_delta)
            dist_coauth_delta = coauth_delta; //this can happen in early years
        
        picked_authors[i]->assignCoauthExp((unsigned)coauth_delta);
        picked_authors[i]->assignDCoauthExp((unsigned)dist_coauth_delta);
        
    } //for

    
    //getting numbers for authors per paper expectation...
    GaussianDistribution probAuth(1,Functions::probAuthExpectation(year),
        Functions::probAuthVariance(year));    
    
    
    /* plan: handling papers one by one, trying to satisfy
        distinct co-author property for all selected authors as early as possible, then
        total co-authors */
    vector <unsigned> first_poses; //positions to start searching in
        //authors vecor for papers requiring INDEX distinct co authors
        //(gets initialized to 0 for each index when first used, then
        //increases while we're learning from failure)

    for(unsigned i = 0; i < publication_pool.count(); i++)
    {
    unsigned auth_num = probAuth.getRandomnizedValue();

        /* picking auth_num authors (preferring those who can accept at least
           auth_num new co authors) */
        vector <Author *> workgroup;
        vector <unsigned> nondistinct;
        int wanted_distinct = auth_num-1;

        do
        {
            /* trying to find author who expects at least wanted_distinct new distinct 
               co authors AND who is distinct to all previously selected authors, OR
               who is not distinct to all of them but expects at least "delta" more
               non-distinct co authors (and all previously choosen authors also have
               a pending delta of at least one) */
            while(first_poses.size() < (unsigned)wanted_distinct + 1)
                first_poses.push_back(0);
            unsigned first_possible_pos = first_poses[wanted_distinct];
                //first pos where we could
                //find an author with expected disctinct coAuths >=
                //wanted_distinct

            for(unsigned ii = first_possible_pos; ii < picked_authors.size()
                && workgroup.size() < auth_num; ii++)
            {
            
                //expecting at least wanted_distinct (additional) new distinct co authors this year?
                //also need to check whether one or more publications are expected for this author
                if(picked_authors[ii]->expectedDCoauths() >= (unsigned)wanted_distinct
                    && picked_authors[ii]->expectedPublications())
                {
                    //distinct to how many of the previously picked authors? - and (where not)
                    //could those other authors accept one more non-distinct author?
                    bool distinct_to_all = true;
                    bool acceptable = true;
                    for(unsigned iii = 0; iii < workgroup.size(); iii++)
                    {
                        if(workgroup[iii] == picked_authors[ii])
                        { //can't have the same author twice
                            acceptable = false;
                            break;
                        }
                        
                        if(workgroup[iii]->haveCoAuthor(picked_authors[ii]))
                        {
                            distinct_to_all = false;
                            if(! nondistinct[iii])
                            {
                                acceptable = false;
                                break;
                            } //if
                        } //if
                    } //for
                    
                    //satisfies all criteria or can be accepted through accepting
                    //non-distinct everywhere?
                    if(acceptable)
                    {
                        //adding author to working group
                        workgroup.push_back(picked_authors[ii]);
                        nondistinct.push_back(picked_authors[ii]->expectedCoauths()
                            - picked_authors[ii]->expectedDCoauths());
                        
                        //decreasing number of (remaining) expected co authors/distinct
                        //co authors for everyone in the working group:
                        for(unsigned iii = 0; iii < workgroup.size() - 1; iii++)
                            if(workgroup[iii]->haveCoAuthor(picked_authors[ii]))
                            { //non-distinct
                                nondistinct[iii]--;
                                nondistinct[nondistinct.size()-1]--;
                            }
                    } //if(acceptable)
                    
                } //if
                else if(first_poses[wanted_distinct] < ii)
                {
                    first_poses[wanted_distinct] = ii;
                }
            } //for
            
            if(wanted_distinct == 0 && workgroup.size() < auth_num)
            { //it all didn't help - need to add some totally new authors, to keep
                //error as small as possible
                
                //adding missing authors as new:
                unsigned delta = auth_num - workgroup.size();
                auths->insertNew(delta, picked_authors, year);

                //assigning all of them with one expected paper, auth_num-1 expected
                //co authors, adding to working group
                for(unsigned ii = picked_authors.size() - delta;
                    ii < picked_authors.size(); ii++)
                {
                    picked_authors[ii]->assignPublExp(1);
                    picked_authors[ii]->assignCoauthExp(auth_num-1);
                    picked_authors[ii]->assignDCoauthExp(auth_num-1);
                    
                    workgroup.push_back(picked_authors[ii]);
                    nondistinct.push_back(0); //it's all 0 for them
                }
            } //if
            
            /* the "n" distinct expected property is not satisfiable anymore with this
               algorithm, step down by one to go ahead and keep error as small as possible:*/
            wanted_distinct--;

        } while(workgroup.size() < auth_num);

        //now we have a working group for this publication assigned:
        for(unsigned ii = 0; ii < workgroup.size(); ii++)
        {
            //assigning co authors to each other (thereby decreasing number of
            //(remaining) expected co authors/distinct co authors
            for(unsigned iii = 0; iii < workgroup.size(); iii++)
                if(iii != ii)
                    workgroup[ii]->addCoAuthor(workgroup[iii]);
                    
            //adding publication/decreasing publication expectation
            workgroup[ii]->addPublication();
            
            //relating all authors with the publication (triple printing relation, non-dependant)
            publication_pool.getAt(i)->addRelated(workgroup[ii], false);
            
            //creating the paper-author relation triples and relating them
            //to papers (dependant relation)
            publication_pool.getAt(i)->addRelated(
                workgroup[ii]->createPublicationRelationTriple(publication_pool.getAt(i)),
                true);

        } //for(working group)

        if(pa_yr_cnt < 10 && year <= 1996) //for ten papers each year, up to 1996
        {
            BlindRDFTriple *bt = new BlindRDFTriple(powerauthor_uri, "", "");
                publication_pool.getAt(i)->addRelated(new PropertyAuthor(
                    publication_pool.getAt(i), bt), true);

            delete bt;
    
            pa_yr_cnt++;
        }

    } //for(all papers)
    
    //powerauthor as editor:
    for(unsigned pa_ed = 0; pa_ed < 2; pa_ed++)
    {
        if(year > 1996) break;

        if(c_pool.count() <= pa_ed)
            break;

        c_pool.getAt(pa_ed)->addRelated(new PropertyEditor(
            c_pool.getAt(pa_ed), powerauthor_uri), true);
    }


    #ifdef LOGMODE
    vector <unsigned> real_a_with_publ;
    for(unsigned i = 0; i < picked_authors.size(); i++)
    {
        //calculating number of publications in this year
        unsigned publ_num = picked_authors[i]->origExpectedPublications()
            - picked_authors[i]->expectedPublications();
    
        //make sure that vector is large enough
        while(real_a_with_publ.size() <= publ_num)
            real_a_with_publ.push_back(0);
        
        real_a_with_publ[publ_num]++; //increase for this author
    } //for

    char *fn = StringTools::numStr("log/publicationsperauthor", year);
    FILE *f = fopen(fn, "a+");
    delete[] fn;
    
    if(f == 0L)
    {
        cout << "fatal error: failed to access log file in ./log/ for writing" << endl;
        exit(2);
    }
    
    unsigned real=0;
    for(unsigned i = 0; i < real_a_with_publ.size()
        && i < authors_with_publs_copy.size(); i++)
    {
    unsigned exp_num = 0;
    unsigned real_num = 0;
    
        if(i < authors_with_publs_copy.size() && i > 0)
            exp_num = authors_with_publs_copy[i-1];
        
        if(i < real_a_with_publ.size())
            real_num = real_a_with_publ[i];
        
        char *cs = StringTools::numStr("", i);
        char *ens = StringTools::numStr(" ", exp_num);
        char *rns = StringTools::numStr(" ", real_num);

        fputs(cs, f);
        fputs(ens, f);
        fputs(rns, f);
        fputs("\n", f);
        
        delete[] cs;
        delete[] ens;
        delete[] rns;
        real+=i*real_a_with_publ[i];
    } //for
    char *rs = StringTools::numStr("",real);
	delete rs;
    fputs("### TOTAL NUMBER OF AUTHORS = ", f);
    fputs(rs, f);
    fputs("\n", f);
    
    fclose(f);
    #endif

    //adding editors...
    vector <unsigned> editors_with_publs; //editors_with_publs[i] = number of editors with
        //i publications as an author

    for(unsigned i = 0; Functions::editorPublications(i, year); i++)
        editors_with_publs.push_back(Functions::editorPublications(i, year));

    if(editors_with_publs.size() == 0)
        editors_with_publs.push_back(1); //required fallback to have
            //at least one editor at hand
            
    //distribution of repeated editor attribute
    GaussianDistribution prob_edit(1,Functions::probEditExpectation(year),
        Functions::probEditVariance(year));

    vector <Editor *> editors;
    
    //finding appropriate editors from pool (or creating new ones for editors
    //with no publications
    for(unsigned i = 0; i < editors_with_publs[0]; i++)
        //editors with no publications as an author
    {
        Author *dummy = new Author(year);
        editors.push_back(new Editor(dummy,true));
    } //for
    
    for(unsigned i = 1; i < editors_with_publs.size(); i++)
    { //editors with i publications as an author
        
        //getting authors with precise number of publications
        vector <Author *> a_with_n_publ = auths->getMAuthorsWithNPublications(
            editors_with_publs[i], i);
            
        for(unsigned ii = 0; ii < a_with_n_publ.size(); ii++)
            editors.push_back(new Editor(a_with_n_publ[ii]));
            
        //if none (not enough) were available, again use dummies...
        for(unsigned ii = 0; ii < editors_with_publs[i] - a_with_n_publ.size(); ii++)
        {
            Author *dummy = new Author(year);
            editors.push_back(new Editor(dummy,true));
        }

    } //for

    //editor's pool created, now pooling relevant papers and assigning to papers...
    //pooling all publications that might require an editor
    RDFTriplePool editor_publication_pool;
    editor_publication_pool.addPool(&c_pool); //proceedings
    editor_publication_pool.addPool(&j_pool); //journals
    editor_publication_pool.addPool(&other_pool); //others (includes relevant categories
        //"books" and "wwws"
    
    //looping relevant papers...
    unsigned ed_pos = 0;

    for(unsigned i = 0; i < editor_publication_pool.count(); i++)
    {
        if(editor_publication_pool.getAt(i)->requirement(2))
            //requirement(1) == Document::requireEditor()
        {
        unsigned editor_cnt = prob_edit.getRandomnizedValue();

            // enforce (avl editors>=editor_cnt), which may fail in early years
            while (editors.size()<editor_cnt)
            {
                Author *dummy = new Author(year);
                editors.push_back(new Editor(dummy,true));
            }

            //now assigning authors to paper
            for(unsigned ii = 0; ii < editor_cnt; ii++)
            {
                editor_publication_pool.getAt(i)->addRelated(
                    editors[ed_pos]->getRelationTriple(
                        editor_publication_pool.getAt(i)));

                ed_pos++;
                
                if(ed_pos >= editors.size()) ed_pos = 0;
            } //for

        } //if(require editor)
        
    } //for

    //citations
    //number of outgoing citations (distribution; call once for every paper
    // which requires a cite)
    GaussianDistribution prob_cite(1,Functions::probCiteExpectation(year),
        Functions::probCiteVariance(year));
        
    //pooling papers that might require citations
    RDFTriplePool citepapers_pool;
    citepapers_pool.addPool(&t_pool); //inproceedings
    citepapers_pool.addPool(&a_pool); //articles
    citepapers_pool.addPool(&other_pool); //others

    //now assigning outgoing citations...
    unsigned requiring=0;
    for(unsigned i = 0; i < citepapers_pool.count(); i++)
    {
        if(citepapers_pool.getAt(i)->requirement(1)) //== Document::requireCite()
        {
        requiring++;
        unsigned cite_cnt = prob_cite.getRandomnizedValue(); //number of outgoing citations
        
            vector <char *> uris = missingcites.citeSomeURIs(cite_cnt);
            
            ReferenceBag *ref_bag = new ReferenceBag();
            ReferenceBagReference *c = new ReferenceBagReference(
                        citepapers_pool.getAt(i)->getSubjectPtr()->getString(),
                        ref_bag->getSubjectPtr()->getString());
            c->addRelated(ref_bag, true);
            citepapers_pool.getAt(i)->addRelated(c, true);

            unsigned cnum = 1;
            while(cite_cnt)
            {
                PropertyCite *ci;
                
                char * uri = 0L;
                if(uris.size() > 0)
                {
                    uri = uris[uris.size() -1];
                    uris.pop_back();
                }

                if(uri)
                {
                    ci = new PropertyCite(
                        ref_bag->getSubjectPtr()->getString(), cnum, uri);
                    
                    delete[] uri;
                }
                else
                {
                    ci = new PropertyCite(
                        ref_bag->getSubjectPtr()->getString(),
                        cnum, "<http://localhost/misc/UnknownDocument>");
                }
                
                citepapers_pool.getAt(i)->addRelated(ci, true);

                cite_cnt--;
                cnum++;

            } //while
        } //if
    } //for


    /*incoming cites (for some papers) - number of papers with N expected incoming cites;
      nrIncomingCitations() returns the total (historic) number of papers with expected
      N incoming citations.
      As most of the cites that we request this year will be assigned next year, the best
      approximation will be reached by assigning expectations one year earlier.
      We also add up and remember the total number of new papers from this year which
      expect incoming cites, so we can determine a percentage of papers to nominate for
      future incoming cites */
    vector <unsigned> papers_with_cites;
    unsigned total_papers_with_incoming_cites = 0;

    // estimate total nr of outgoing references in next year;
    // we will then use this value to generate a certain percentage of 
    // incoming citation (the other outgoing citations point to
    // UnknownDocument, i.e. refere documents not contained in the data
    // set, just like the original DBLP data set)
    unsigned est_nr_ref=Functions::estimateNrOutgoingReferencesUpTo(year+1,
                                            total_nr_inproceedings,
                                            total_nr_proceedings,
                                            total_nr_articles,
                                            total_nr_journals,
                                            total_nr_incollections,
                                            total_nr_books,
                                            total_nr_wwws,
                                            total_nr_masterstheses,
                                            total_nr_phdtheses,
											prob_cite.getAverage());
    double c_param=Functions::fitNrIncomingCitationsParameter(year+1,est_nr_ref);

    // estimate last year again (we require this again)
    unsigned est_nr_ref_lastyear=Functions::estimateNrOutgoingReferencesUpTo(year,
                                            total_nr_inproceedings-Functions::inproc(year),
                                            total_nr_proceedings-Functions::proc(year),
                                            total_nr_articles-Functions::article(year),
                                            total_nr_journals-Functions::journal(year),
                                            total_nr_incollections-Functions::incoll(year),
                                            total_nr_books-Functions::book(year),
                                            total_nr_wwws-Functions::www(year),
                                            total_nr_masterstheses-Functions::master(year),
                                            total_nr_phdtheses-Functions::phd(year),
											prob_cite.getAverage());
    double c_param_lastyear=Functions::fitNrIncomingCitationsParameter(year,est_nr_ref_lastyear);

    unsigned total_incoming_citations=0;
    for(unsigned i = 1; Functions::nrIncomingCitations(i, year+1, c_param); i++)
    {
    unsigned exp = Functions::nrIncomingCitations(i,year+1,c_param);
    unsigned prev = Functions::nrIncomingCitations(i,year,c_param_lastyear);

        if(exp > prev) {
            exp -= prev;
        } else {
            exp = 0;
        }

        papers_with_cites.push_back(exp);
        total_papers_with_incoming_cites += exp;
        total_incoming_citations+=exp*i;
    }
    total_incoming_cites+=total_incoming_citations;

    //loop again for expected number of (future) incoming citations:
    for(unsigned i = 0; i < citepapers_pool.count(); i++)
    {
        //some papers will also be honoured by incoming citations (in future) ;-)
        if(i >= total_papers_with_incoming_cites)
            break;
        else//(r->getRandomValue(100) <= percent_with_incoming_cites)
        {
            //assigning number of expected incoming cites
            unsigned cinum = 1;
            
            for(unsigned ii = 0; ii < papers_with_cites.size(); ii++)
                if(papers_with_cites[ii])
                {
                    cinum = ii + 1;
                    papers_with_cites[ii]--;
            
                    //adding to list of URI's to which we expect incoming citations later...
                    missingcites.request(
                        citepapers_pool.getAt(i)->getSubjectPtr()->getString(), cinum);
                        //need URI only, plus expected number of incoming citations
                        
                    break;
                }
        }
    } //for

    cout << "[" << year << "] all contents calculated, now storing...\n";

    /* preparing to write output, starting with conferences/journals/other docs,
        always filling a conference etc completely before proceeding...
        checking limits, skipping some if necessary;
        expect to have all events/publications distributed evenly over the year... */
    float perc_c = 0.0, perc_j = 0.0, perc_o = 0.0; //percentage of conferences,
        //journals (every issue counts), other documents already written

    //percentage delta for each event/publication
    float pd_c = 0;
    if(c_pool.count())
        pd_c = 100.0 / c_pool.count();
    else
        perc_c = 100.0;

    float pd_j = 0;
    if(j_pool.count())
        pd_j = 100.0 / j_pool.count();
    else
        perc_j = 100.0;

    float pd_o = 0;
    if(other_pool.count())
        pd_o = 100.0 / other_pool.count();
    else
        perc_o = 100.0;

    //while some stuff still needs 2b stored...
    while((c_pool.count() || j_pool.count() || other_pool.count())
        && ! olm.outputComplete())
    {    
        //we try to keep the percentage of all events/singular publications at a
        //similar level, so in case we reach some output limit, their distribution is
        //still realistic
        if(perc_c <= perc_j
            && perc_c <= perc_o)
        {
            //storing unprinted relevant triples
            RDFTriple * t = c_pool.getElement();
            RDFTriplePool unprinted_triples(t->recursivePool());
            vector<char *> lines = unprinted_triples.printAll();
            unsigned nr_triples=store(lines);
            for(unsigned i = 0; i < lines.size(); i++)
                delete[] lines[i];
            c_pool.removeElement(t);

            //updating limit manager
            //olm.add(unprinted_triples.count(),
            olm.add(nr_triples,unprinted_triples.totalSize());
            unprinted_triples.uncalcAll();
            
            //updating progress percentage
            perc_c += pd_c;

            //cleaning up
            delete t;
        }
        else if(perc_j <= perc_o)
        {
            //storing unprinted relevant triples
            RDFTriple * t = j_pool.getElement();
            RDFTriplePool unprinted_triples(t->recursivePool());
            vector<char *> lines = unprinted_triples.printAll();
            unsigned nr_triples=store(lines);
            for(unsigned i = 0; i < lines.size(); i++)
                delete[] lines[i];
            j_pool.removeElement(t);
            
            //updating limit manager
            //olm.add(unprinted_triples.count(),
            olm.add(nr_triples,unprinted_triples.totalSize());
            unprinted_triples.uncalcAll();
            
            //updating progress percentage
            perc_j += pd_j;
        }
        else //publication ("other" publication)
        {
            //storing unprinted relevant triples
            RDFTriple * t = other_pool.getElement();
            RDFTriplePool unprinted_triples(t->recursivePool());
            vector<char *> lines = unprinted_triples.printAll();
            unsigned nr_triples=store(lines);
            for(unsigned i = 0; i < lines.size(); i++)
                delete[] lines[i];
            other_pool.removeElement(t);

            //updating limit manager
            //olm.add(unprinted_triples.count(),
            olm.add(nr_triples,unprinted_triples.totalSize());
            unprinted_triples.uncalcAll();
            
            //updating progress percentage
            perc_o += pd_o;
        }

    } //while

    garbage_pool.addPool(&c_pool);
    garbage_pool.destroyElements();

    //authors who retire after this year:
    #ifdef LOGMODE
    cout << "[" << year << "] Active authors=" << auths->count() << endl;
    #endif // LOGMODE
    unsigned anum_retiring = Functions::nrRetiringAuthors(year,auths->count());
    #ifdef LOGMODE
    cout << "[" << year << "] Retiring " << anum_retiring << " authors now..." << endl;
    #endif // LOGMODE
    auths->retire(anum_retiring);
    #ifdef LOGMODE
    cout << "[" << year << "] Remaining authors after retiring=" << auths->count() << endl;
    #endif // LOGMODE

    //dumping editor objects
    for(unsigned i = 0; i < editors.size(); i++)
        delete editors[i];

    #ifdef LOGMODE
    char *fn_coauths = StringTools::numStr("log/histcoauthavg", year);
    f = fopen(fn_coauths, "a+");
    delete[] fn_coauths;

    //log avg number of co authors (life time) for all active authors (non-retired)
    auths->averageCoAuthors();
    
    //log avg number of distinct co authors (life time) for all active authors (non-retired)
    auths->averageDistinctCoAuthors();
    
    fclose(f);
    
    InstanceLogger *l = InstanceLogger::getInstance();
    l->writeLogs(year);
    
    vector<unsigned> incoming_cites = missingcites.getIncomingCiteCounts();
    char *fn_cites = StringTools::numStr("log/citations", year);
    f = fopen(fn_cites, "a+");
    delete[] fn_cites;
    
    if(f == 0L)
    {
        cout << "fatal error: failed to access log file in ./log/ for writing" << endl;
        exit(2);
    }
    
    for(unsigned i = 0; i < incoming_cites.size(); i++)
    {
        char *cns = StringTools::numStr("", i+1);
        char *cis = StringTools::numStr(" ", incoming_cites[i]);

        fputs(cns, f);
        fputs(cis, f);
        fputs("\n", f);

        delete[] cns;
        delete[] cis;
    }
    
    fclose(f);
    #endif //LOGMODE

} //generateYear()


/*!
    \fn Generator::generateConferences(unsigned)
    generates all conference objects for specified year, including the basic attributes
    (added as related triples)
    @return pool with generated triple objects
 */
RDFTriplePool Generator::generateConferences(unsigned year,unsigned & total_nr_proceedings)
{
RDFTriplePool ret;
unsigned cnt = Functions::proc(year);

    // adapt count
    total_nr_proceedings+=cnt;

    for(unsigned i = 0; i < cnt; i++)
    {
    Proceedings * e = new Proceedings(i+1, year);
        ret.add(e);
    }
    
    return ret;
} //generateConferences()

/*!
    \fn Generator::generateJournals(unsigned)
    generates all journal objects for specified year, including the basic attributes
    (added as related triples)
    @return pool with generated triple objects
 */
RDFTriplePool Generator::generateJournals(unsigned year,unsigned & total_nr_journals)
{
RDFTriplePool ret;
unsigned cnt = Functions::journal(year);

    // adapt count
    total_nr_journals+=cnt;

    for(unsigned i = 0; i < cnt; i++)
    {
    Journal * e = new Journal(i+1, year);
        ret.add(e);
    }
    
    return ret;
} //generateJournals()

/*!
    \fn Generator::generateInProceedings(unsigned)
    generates all objects of InProceedings publications
    for specified year, including the basic attributes
    (added as related triples)
    @param year current year
    @param c_pool pointer to pool with available conferences
    @return pool with generated triple objects
 */
RDFTriplePool Generator::generateInProceedings(unsigned year, RDFTriplePool *c_pool,
                                                unsigned & total_nr_inproceedings)
{
RDFTriplePool ret;


    if(! c_pool->count())
        return ret; //can have no inproceedings without proceedings

    unsigned cnt = Functions::inproc(year);
    total_nr_inproceedings+=cnt;

    vector <unsigned> conference_sizes; //expectation values for conference sizes
    GaussianDistribution g(1, cnt/c_pool->count(), .2 * cnt/c_pool->count());

    //initializing expectation values for conference sizes...
    for(unsigned i = 0; i < c_pool->count(); i++)
    {
        conference_sizes.push_back(g.getRandomnizedValue());
    } //for

    unsigned proceeding_num = 0;
    
    for(unsigned i = 0; i < cnt; i++)
    {
        //distributing papers over conferences (exp. sizes); if sum of exp. vals is a bit
        //too small, just rotate equally from beginning for remaining talks...
        if(conference_sizes[proceeding_num])
            conference_sizes[proceeding_num]--;
        else
            proceeding_num++;
            
        if(proceeding_num >= c_pool->count())
            proceeding_num = 0; 
        
        Inproceedings * e = new Inproceedings(i+1, year, proceeding_num+1);
    
        //as it happens, this numeric id part is equal to pos of conference in pool
        c_pool->getAt(proceeding_num)->addRelated(e,false);
        
        if(e->requireCrossRef())
        {
            RDFTriple *t = new PropertyCrossref(e, c_pool->getAt(proceeding_num));
            e->addRelated(t, true);
        }
        
        ret.add(e);
    } //for
    
    return ret;

} //generateInProceedings()


/*!
    \fn Generator::generateArticles(unsigned)
    generates all objects of article publications
    for specified year, including the basic attributes
    (added as related triples)
    @param year current year
    @param j_pool pointer to pool with available journals
    @return pool with generated triple objects
 */
RDFTriplePool Generator::generateArticles(unsigned year, RDFTriplePool *j_pool,
                                            unsigned & total_nr_articles)
{
RDFTriplePool ret;
unsigned cnt = Functions::article(year);

    // adapt counts
    total_nr_articles+=cnt;

    if(! cnt)
        return ret; //to avoid non-necessary (but dangerous) calculations

    if(! j_pool->count() && cnt > 0)
    {
        /* that one is a bit complicated; articles without journals exist, but they're rare,
            so we don't expect the case of having articles but no journals in a year.
            to avoid any misdistribution in output, abort in case it should happen, though ;-) */
        cout << "state with invalid constellation of journals/articles reached" << endl;
        exit(8);
    }

    vector <unsigned> journal_sizes; //expectation values for conference sizes

    GaussianDistribution g(1, cnt/j_pool->count(), .2 * cnt/j_pool->count());

    //initializing expectation values for journal sizes...
    for(unsigned i = 0; i < j_pool->count(); i++)
    {
        journal_sizes.push_back(g.getRandomnizedValue());
    } //for

    unsigned journal_num = 0;

    for(unsigned i = 0; i < cnt; i++)
    {
        //distributing papers over journal (exp. sizes); if sum of exp. vals is a bit
        //too small, just rotate equally from beginning for remaining talks...
        if(journal_sizes[journal_num])
            journal_sizes[journal_num]--;
        else
            journal_num++;
            
        if(journal_num >= j_pool->count())
            journal_num = 0; 
        
        Article * e = new Article(i+1, year, journal_num+1);
    
        /*as it happens, this numeric id part is equal to pos of journal in pool + 1
          however it is important to associate only those articles that really have a
          journal attribute */
        if(e->requireJournal())
        {
            j_pool->getAt(journal_num)->addRelated(e,false);
            
            RDFTriple *t = new PropertyJournal(e, j_pool->getAt(journal_num));
            e->addRelated(t, true);
        }
        
        ret.add(e);
        
    } //for

    
    return ret;
} //generateArticles()


/*!
    \fn Generator::generateOthers(unsigned)
    generates all objects of publications except InProceedings or articles
    for specified year, including the basic attributes
    (added as related triples)
    @return pool with generated triple objects
 */
RDFTriplePool Generator::generateOthers(unsigned year, unsigned &total_nr_incollections,
                                        unsigned &total_nr_books,unsigned &total_nr_wwws,
                                        unsigned &total_nr_masterstheses,
                                        unsigned &total_nr_phdtheses)
{
RDFTriplePool ret;
unsigned books = Functions::book(year);
unsigned incolls = Functions::incoll(year);
unsigned phds = Functions::phd(year);
unsigned masters = Functions::master(year);
unsigned wwws = Functions::www(year);

    // adapt counts
    total_nr_incollections+=incolls;
    total_nr_books+=books;
    total_nr_wwws+=wwws;
    total_nr_masterstheses+=masters;
    total_nr_phdtheses+=phds;
    
    for(unsigned i = 0; i < books; i++)
    {
    Book * e = new Book(i+1, year);
        ret.add(e);
    }

    
    for(unsigned i = 0; i < incolls; i++)
    {
    Incollection * e = new Incollection(i+1, year);
        ret.add(e);
    }

    
    for(unsigned i = 0; i < phds; i++)
    {
    Phdthesis * e = new Phdthesis(i+1, year);
        ret.add(e);
    }
    
    
    for(unsigned i = 0; i < masters; i++)
    {
    Mastersthesis * e = new Mastersthesis(i+1, year);
        ret.add(e);
    }
    
    
    for(unsigned i = 0; i < wwws; i++)
    {
    Www * e = new Www(i+1, year);
        ret.add(e);
    }
    
    return ret;
} //generateOthers()


/*!
    \fn Generator::store(vector <char *>)

    Stores all the lines in triple_lines on the disk (appending to output file)
 */
unsigned Generator::store(vector <char *> triple_lines)
{
    FILE *f = fopen64(cmd->getOutFileName(), "a+");
    
    if(f == 0L)
    {
        cout << "fatal error: failed to access output file '"
				<< cmd->getOutFileName() << "' for writing" << endl;
        exit(2);
    }

    unsigned nr_triples=0;
    for(unsigned i = 0; i < triple_lines.size(); i++)
        if(strlen(triple_lines[i])) //ignore empty/void triples
        {
            fputs(triple_lines[i], f);
            fputs("\n\0", f);
            nr_triples++;
        }

    fclose(f);
    return nr_triples;
}
