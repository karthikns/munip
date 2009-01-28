#include "staff.h"

#include "tools.h"

#include <QDebug>
#include <QLabel>
#include <QRgb>
#include <QSet>
#include <QVector>
#include <cmath>

namespace Munip
{
   StaffLine::StaffLine(const QPoint& start, const QPoint& end, int staffID)
   {
      m_startPos = start;
      m_endPos = end;
      m_staffID = staffID;
      m_lineWidth = -1; // still to be set
   }

   StaffLine::~StaffLine()
   {

   }

   QPoint StaffLine::startPos() const
   {
      return m_startPos;
   }

   void StaffLine::setStartPos(const QPoint& point)
   {
      m_startPos = point;
   }

   QPoint StaffLine::endPos() const
   {
      return m_endPos;
   }

   void StaffLine::setEndPos(const QPoint& point)
   {
      m_endPos = point;
   }

   int StaffLine::staffID() const
   {
      return m_staffID;
   }

   void StaffLine::setStaffID(int id)
   {
      m_staffID = id;
   }

   int StaffLine::lineWidth() const
   {
      return m_lineWidth;
   }

   void StaffLine::setLineWidth(int wid)
   {
      m_lineWidth = wid;
   }


   Staff::Staff(const QPoint& vStart, const QPoint& vEnd)
   {
      m_startPos = vStart;
      m_endPos = vEnd;
   }

   Staff::~Staff()
   {

   }


   QPoint Staff::startPos() const
   {
      return m_startPos;
   }

   void Staff::setStartPos(const QPoint& point)
   {
      m_startPos = point;
   }


   QPoint Staff::endPos() const
   {
      return m_endPos;
   }

   void Staff::setEndPos(const QPoint& point)
   {
      m_endPos = point;
   }

   QList<StaffLine> Staff::staffLines() const
   {
      return m_staffLines;
   }

   void Staff::addStaffLine(const StaffLine& sline)
   {
      m_staffLines.append(sline);
   }

   bool Staff::operator<(Staff& other)
   {
      return m_startPos.y() < other.m_startPos.y();
   }

   StaffLineRemover::StaffLineRemover(Page *page)
   {
      Q_ASSERT(page);
      m_page = page;
      // Initially processedImage = OriginalImage
      m_processedImage = m_page->originalImage();
   }

   StaffLineRemover::~StaffLineRemover()
   {
   }

   void StaffLineRemover::removeLines()
   {
      int startx,starty,endx,endy;
      startx = starty = 0;
      endx = startx + m_processedImage.width();
      endy = starty + m_processedImage.height();

      QVector<bool> parsed(endy - starty, false);

      for(int x = startx; x < endx; x++)
      {
         for(int y = starty; y < endy; y++)
         {
            if (parsed[y] == true) continue;

            MonoImage::MonoColor currPixel = m_page->originalImage().pixelValue(x, y);
            if(currPixel == MonoImage::Black)
            {
               QPoint staffStart(x, y);
               while(currPixel == MonoImage::Black && y < endy)
               {
                  m_processedImage.setPixelValue(x, y, MonoImage::White);
                  parsed[y] = true;
                  y = y+1;
                  if (y < endy && parsed[y+1]) break;
                  // TODO : CHeck if the new y is parsed or not (should we check ? )
                  currPixel = m_page->originalImage().pixelValue(x, y);
               }
               QPoint staffEnd(x, y-1);
               m_staffList.append(Staff(staffStart, staffEnd));
            }
         }
      }

      //qSort(m_staffList.begin(), m_staffList.end());
   }

   /**
    * This is method I(gopal) tried to remove lines. The logic is as
    * follows
    *
    * - Look for a black pixel in a row-wise left-to-right fashion.
    *
    * - Call checkForLine method, which returns a list of points
    *   forming a line. The method returns an empty list if there is
    *   no possibility of lines.
    *
    * - Mark all the above points white.
    *
    * @todo Don't remove points that can be part of symbol.
    */
   void StaffLineRemover::removeLines2()
   {
      // Important: USE m_processedImage everywhere as it has an
      //            updated white-marked image which acts as input
      //            to next stage, thereby reducing false positives
      //            considerably.
      removeLines();
      QRect imgRect = m_processedImage.rect();
      MonoImage display(m_processedImage);
      for(int i = 0; i < imgRect.right(); ++i)
         for(int j = 0; j < imgRect.height(); ++j)
            display.setPixelValue(i, j, MonoImage::Black);


      for(int y = imgRect.top(); y <= imgRect.bottom(); ++y)
      {
         for(int x = imgRect.left(); x <= imgRect.right(); ++x)
         {
            if (m_processedImage.pixelValue(x, y) == MonoImage::Black) {
               //// Some work delegated.
               QList<QPoint> lineSlithers = checkForLine(x, y);

               if (lineSlithers.isEmpty()) {
                  continue;
               }

               int maxX = x, maxY = y;
               foreach(QPoint point, lineSlithers) {
                  maxX = qMax(point.x(), maxX);
                  maxY = qMax(point.y(), maxY);

                  m_processedImage.setPixelValue(point.x(), point.y(), MonoImage::White);
                  display.setPixelValue(point.x(), point.y(), MonoImage::White);

               }
               x = maxX, y = maxY;
            }
         }
      }

      // This shows an image in which only the above detected points
      // are marked white and rest all is black.
      QLabel *label = new QLabel();
      label->setPixmap(QPixmap::fromImage(display));
      label->show();

   }

   /**
    * The logic is as follows.
    *
    * - It first determines a continuous horizontal line (can have
    *   holes in between line, but continuous)
    *
    * - Also nearing points to the above belonging to line (say line
    *   of thickness 3 pixels) are checked and added as part of line.
    *
    * - If the horizontal length is greater than some threshold, then
    *   all the above points are returned.
    */
   QList<QPoint> StaffLineRemover::checkForLine(int x, int y)
   {
      QList<QPoint> slithers;
      slithers.append(QPoint(x, y));
      bool isLine = false;
      QPoint lastSlither(x, y);
      int minY, maxY;
      minY = maxY = y;

      int i = x+1;
      for(; i < m_processedImage.width(); ++i) {
         if (m_processedImage.pixelValue(i, lastSlither.y()) == MonoImage::Black) {
            ;
         }
         else if (lastSlither.y() > 0 && m_processedImage.pixelValue(i, lastSlither.y()-1) == MonoImage::Black) {
            lastSlither.ry()--;
         }
         else if (lastSlither.y() < (m_processedImage.height()-1) &&
                  m_processedImage.pixelValue(i, lastSlither.y() + 1) == MonoImage::Black) {
            lastSlither.ry()++;

         }
         else {
            break;
         }

         lastSlither.rx() = i;
         slithers.append(lastSlither);
         minY = qMin(minY, lastSlither.y());
         maxY = qMax(maxY, lastSlither.y());
      }



      const int ThreasholdLineLength = 80;
      isLine = (i-x) > ThreasholdLineLength;
      if (isLine) {
         // for(int j = x; j < i; ++j) {
         //     for(int k = minY; k <= maxY; ++k) {
         //         if (m_processedImage.pixelValue(j, k) == MonoImage::Black) {
         //             slithers.append(QPoint(j, k));
         //         }
         //     }
         // }
         return slithers;
      }
      else {
         slithers.clear();
         return slithers;
      }
   }

   QImage StaffLineRemover::processedImage() const
   {
      return m_processedImage;
   }

   QList<Staff> StaffLineRemover::staffList() const
   {
      return m_staffList;
   }

   Page::Page(const MonoImage& image) :
			m_originalImage(image),
			m_processedImage(image),
			test(image)
   {
      m_staffSpaceHeight = m_staffLineHeight = -1;
      m_staffLineRemover = 0;
      for(int x = 0; x < image.width(); ++x)
         for(int y = 0; y < image.height(); ++y)
            m_processedImage.setPixelValue(x, y, MonoImage::White);
   }

   Page::~Page()
   {
      delete m_staffLineRemover;
   }

   const MonoImage& Page::originalImage() const
   {
      return m_originalImage;
   }

   const MonoImage& Page::processedImage() const
   {
      return m_processedImage;
   }

   MonoImage Page::staffLineRemovedImage() const
   {
      if (m_staffLineRemover) {
         return m_staffLineRemover->processedImage();
      }
      return MonoImage();
   }

	double Page :: findSkew(std :: vector<QPoint>& points)
	{
		QPointF mean = meanOfPoints(points);
		std::vector<double> covmat = covariance(points, mean);
		if(covmat[1] == 0)
		{
			for(int i = 0; i < points.size(); i++)
				qDebug() << points[i];
		}
		//Q_ASSERT(covmat!= 0);
		double eigenvalue = highestEigenValue(covmat);
     	double skew = (eigenvalue - covmat[0]) / (covmat[1]);
      	double theta = atan((skew));
		return skew;
	
	}

   void Page :: dfs(int x,int y, std :: vector<QPoint> points)
   {
		m_originalImage.setPixelValue(x,y,MonoImage :: White);
		if(points.size() == 20)
		{
			double skew;
			m_skewList.push_back((skew = findSkew(points)));
			while(points.size() > 0)
				points.pop_back();
        }
		if(m_originalImage.pixelValue(x+1,y+1) == MonoImage :: Black && x+1 < m_originalImage.width() &&  y +1< m_originalImage.height())
		{
			points.push_back(QPoint(x,y));
			dfs(x+1,y+1,points);
		}
		if(m_originalImage.pixelValue(x+1,y) == MonoImage :: Black &&  x+1 < m_originalImage.width())
		{
			points.push_back(QPoint(x,y));
			dfs(x+1,y,points);
		}
		if(m_originalImage.pixelValue(x+1,y-1) == MonoImage :: Black && y -1 > 0)
		{
			points.push_back(QPoint(x,y));
			dfs(x+1,y-1,points);
		}
	}

   double Page::detectSkew()
   {
	  int flag = 1;
	  int x = 0,y = 0;
      for( x = 0;  x < m_originalImage.width();x++)
      {
         for( y = 0; y < m_originalImage.height();y++)
         {
            if(m_originalImage.pixelValue(x,y) == MonoImage :: Black)
     		{	
				std :: vector<QPoint> t;	 
				dfs(x,y,t);
			}
			
         }
		 
      }
	  /*Computation of the skew with highest frequency*/
	  sort(m_skewList.begin(),m_skewList.end());
	  for(int i = 0; i < m_skewList.size(); i++)
	  		qDebug()<<m_skewList[i];

	  int i = 0,n = m_skewList.size();
	  int modefrequency = 0;
	  int maxstartindex,maxendindex;	
	  double modevalue;
	  while( i <= n-1)
	  {
		int runlength = 1;
		double t = m_skewList[i];
		int runvalue = (int) (t * 100);
		while( i + runlength <= n-1 && (int)(m_skewList[i+runlength]*100) == runvalue)
			runlength++;
		if(runlength > modefrequency)
		{
			modefrequency = runlength;
			maxstartindex = i;
			maxendindex = i + runlength;
		}
		i += runlength;
	  }
		double skew = 0;
	  for(int i = maxstartindex; i<= maxendindex;i++)
	  		skew += m_skewList[i];
	  skew /= modefrequency;
			
	  qDebug() << Q_FUNC_INFO <<skew;
	  return skew;
   }

   void Page::correctSkew()
   {
     double theta = std::atan(detectSkew());
      if(theta == 0.0)
         return;


      if (1) {
         for(int y = 0;  y < test.height();y++)
         {
            for(int x = 0; x < test.width();x++)
            {
               if(test.pixelValue(x,y) == MonoImage :: Black)
               {
		 int x1 = qRound(x * cos(theta) + y * sin(theta));
		 int y1 = qRound(-x * sin(theta) + y * cos(theta));
                  qDebug() << Q_FUNC_INFO << QPoint(x1, y1);
                  m_processedImage.setPixelValue(x1,y1,MonoImage :: Black);
               }
            }
         }
      }
      qDebug() << Q_FUNC_INFO << theta;

   }


   QPointF Page::meanOfPoints(const std::vector<QPoint>& pixels) const
   {
      QPointF mean;

      for( unsigned int i = 0; i < pixels.size(); ++i )
      {
         mean.rx() += pixels[i].x();
         mean.ry() += pixels[i].y();
      }

      if(pixels.size()==0)
         return mean;

      mean /= pixels.size();
      return mean;
   }


   std::vector<double> Page::covariance(const std::vector<QPoint>& blackPixels, QPointF mean) const
   {
      std::vector<double> varianceMatrix(4, 0);
      double &vxx = varianceMatrix[0];
      double &vxy = varianceMatrix[1];
      double &vyx = varianceMatrix[2];
      double &vyy = varianceMatrix[3];


      for( unsigned int i=0; i < blackPixels.size(); ++i )
      {
         vxx += ( blackPixels[i].x() - mean.x() ) * ( blackPixels[i].x() - mean.x() );
         vyx = vxy += ( blackPixels[i].x() - mean.x() ) * ( blackPixels[i].y() - mean.y() );
         vyy += ( blackPixels[i].y() - mean.y() ) * ( blackPixels[i].y() - mean.y() );
      }

       vxx /= blackPixels.size();


       vxy /= blackPixels.size();
       vyx /= blackPixels.size();
       vyy /= blackPixels.size();

      return varianceMatrix;
   }


   double Page::highestEigenValue(const std::vector<double> &matrix) const
   {
      double a = 1;
      double b = -( matrix[0] + matrix[3] );
      double c = (matrix[0]  * matrix[3]) - (matrix[1] * matrix[2]);
      double D = b * b - 4*a*c;
      //qDebug() << a << b << c << D;
      Q_ASSERT(D >= 0);
      double lambda = (-b + std :: sqrt(D))/(2*a);
      return lambda;

   }


   /**
    * This method calls the required processing techniques in proper
    * sequence.
    */
   void Page::process()
   {
      if (!m_staffLineRemover) {
         m_staffLineRemover = new StaffLineRemover(this);
      }

      m_staffLineRemover->removeLines2();
   }
}
